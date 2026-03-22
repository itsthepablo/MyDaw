#include "MainComponent.h"
#include "Bridge/TrackPianoRollBridge.h"
#include "Bridge/TrackEffectsBridge.h"
#include "Bridge/TrackMixerPlaylistBridge.h"
#include "Bridge/TransportBridge.h" 
#include "Bridge/InterfaceBridge.h"

MainComponent::MainComponent() {
    commandManager.registerAllCommandsForTarget(this);
    addKeyListener(commandManager.getKeyMappings());

    resourceMeter = std::make_unique<ResourceMeter>(*this);

    addAndMakeVisible(transportBar);
    addAndMakeVisible(toolbarButtons);
    addAndMakeVisible(*resourceMeter); 
    
    addAndMakeVisible(pickerPanelUI); // AÑADIDO
    addAndMakeVisible(effectsPanelUI);
    addAndMakeVisible(trackContainer);
    addAndMakeVisible(playlistUI);
    addAndMakeVisible(mixerUI);

    mixerUI.setVisible(false);
    effectsPanelUI.setVisible(false);
    pickerPanelUI.setVisible(true);

    trackContainer.setExternalMutex(&audioMutex);
    playlistUI.setExternalMutex(&audioMutex);
    
    // Le damos acceso a las pistas para que las pueda escanear
    pickerPanelUI.setTrackContainer(&trackContainer);
    
    toolbarButtons.onSnapChanged = [this](double newSnapPixels) {
        playlistUI.snapPixels = newSnapPixels;
    };

    toolbarButtons.onToolChanged = [this](int toolId) {
        playlistUI.setTool(toolId);
    };

    playlistUI.onMidiClipDeleted = [this](MidiClipData* deletedClip) {
        if (pianoRollUI.getActiveNotesPointer() == &(deletedClip->notes)) {
            pianoRollUI.setActiveNotes(nullptr);
            if (pianoRollWindow) pianoRollWindow->setVisible(false);
        }
    };

    TrackPianoRollBridge::connect(trackContainer, playlistUI, pianoRollUI, pianoRollWindow);
    TrackPianoRollBridge::connectPlaylist(playlistUI, pianoRollUI, pianoRollWindow);

    TrackEffectsBridge::connect(trackContainer, effectsPanelUI, audioMutex,
        audioEngine.clock.sampleRate, audioEngine.clock.blockSize,
        isEffectsPanelVisible, [this] { resized(); });

    TrackMixerPlaylistBridge::connect(trackContainer, mixerUI, playlistUI);
    TransportBridge::connect(transportBar, pianoRollUI, playlistUI);

    // ACTALIZADO EL BRIDGE
    InterfaceBridge::connect(toolbarButtons, isMixerVisible, isEffectsPanelVisible, isPickerVisible,
        mixerUI, effectsPanelUI, pickerPanelUI, trackContainer, [this] { resized(); });

    trackContainer.onDeleteTrack = [this](int index) {
        const juce::ScopedLock sl(audioMutex);
        if (index >= 0 && index < trackContainer.getTracks().size()) {
            TrackPianoRollBridge::cleanup(pianoRollUI, pianoRollWindow, trackContainer.getTracks()[index]);
            trackContainer.removeTrack(index);
            playlistUI.updateScrollBars();
            playlistUI.repaint();

            isEffectsPanelVisible = false;
            effectsPanelUI.setVisible(false);
            resized();
        }
        };

    setSize(1200, 800);
    setAudioChannels(0, 2);
}

MainComponent::~MainComponent() {
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samples, double s) {
    audioEngine.prepareToPlay(samples, s, trackContainer, audioMutex);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
    audioEngine.processBlock(bufferToFill, trackContainer, pianoRollUI, playlistUI, mixerUI, audioMutex);
}

void MainComponent::releaseResources() {
    audioEngine.releaseResources();
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(30, 32, 35));
}

void MainComponent::resized() {
    auto area = getLocalBounds();

    auto topArea = area.removeFromTop(45);
    
    toolbarButtons.setBounds(topArea.removeFromRight(550)); 
    resourceMeter->setBounds(topArea.removeFromRight(100).reduced(8, 10)); 
    transportBar.setBounds(topArea);

    if (isMixerVisible) {
        mixerUI.setVisible(true);
        mixerUI.setBounds(area.removeFromBottom(getHeight() * 0.35));
    }
    else {
        mixerUI.setVisible(false);
    }

    // --- REORGANIZACIÓN DE PANELES LATERALES ---
    
    // 1. Picker Panel (Extremo Izquierdo)
    if (isPickerVisible) {
        pickerPanelUI.setVisible(true);
        pickerPanelUI.setBounds(area.removeFromLeft(160));
    }
    else {
        pickerPanelUI.setVisible(false);
    }

    // 2. Panel de Efectos (Al lado del Picker, si está abierto)
    if (isEffectsPanelVisible) {
        effectsPanelUI.setVisible(true);
        effectsPanelUI.setBounds(area.removeFromLeft(220));
    }
    else {
        effectsPanelUI.setVisible(false);
    }

    // 3. Contenedor de Pistas
    trackContainer.setBounds(area.removeFromLeft(250));
    
    // 4. Playlist
    playlistUI.setBounds(area);
}

juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget() { return nullptr; }
void MainComponent::getAllCommands(juce::Array<juce::CommandID>& c) { c.add(playStopCommand); }

void MainComponent::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& result) {
    if (id == playStopCommand) {
        result.setInfo("Play / Stop", "Transporte", "Transport", 0);
        result.addDefaultKeypress(' ', 0);
    }
}

bool MainComponent::perform(const juce::ApplicationCommandTarget::InvocationInfo& info) {
    if (info.commandID == playStopCommand) {
        transportBar.playBtn.triggerClick();
        return true;
    }
    return false;
}