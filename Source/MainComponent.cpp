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
    
    addAndMakeVisible(leftSidebar); 
    addAndMakeVisible(sidebarResizer); // AÑADIDO A LA PANTALLA
    addAndMakeVisible(trackContainer);
    addAndMakeVisible(playlistUI);
    addAndMakeVisible(mixerUI);

    mixerUI.setVisible(false);
    leftSidebar.setVisible(true);

    trackContainer.setExternalMutex(&audioMutex);
    playlistUI.setExternalMutex(&audioMutex);
    
    pickerPanelUI.setTrackContainer(&trackContainer);
    
    // --- LÓGICA DE REDIMENSIONAMIENTO EN TIEMPO REAL ---
    sidebarResizer.onResizeDelta = [this](int delta) {
        // jlimit evita que el usuario lo haga invisible (mínimo 150px) 
        // o que se coma toda la pantalla (máximo 600px).
        leftSidebarWidth = juce::jlimit(150, 600, leftSidebarWidth + delta);
        resized(); // Forzamos un redibujado instantáneo
    };

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
        [this] { 
            isLeftSidebarVisible = true;
            leftSidebar.showTab(LeftSidebar::FxTab);
            resized(); 
        });

    TrackMixerPlaylistBridge::connect(trackContainer, mixerUI, playlistUI);
    TransportBridge::connect(transportBar, pianoRollUI, playlistUI);

    InterfaceBridge::connect(toolbarButtons, isMixerVisible, isLeftSidebarVisible,
        mixerUI, effectsPanelUI, leftSidebar, trackContainer, [this] { resized(); });

    trackContainer.onDeleteTrack = [this](int index) {
        const juce::ScopedLock sl(audioMutex);
        if (index >= 0 && index < trackContainer.getTracks().size()) {
            TrackPianoRollBridge::cleanup(pianoRollUI, pianoRollWindow, trackContainer.getTracks()[index]);
            trackContainer.removeTrack(index);
            playlistUI.updateScrollBars();
            playlistUI.repaint();

            if (leftSidebar.getCurrentTab() == LeftSidebar::FxTab) {
                isLeftSidebarVisible = false;
            }
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

    if (isLeftSidebarVisible) {
        leftSidebar.setVisible(true);
        sidebarResizer.setVisible(true); // Mostramos el divisor
        
        // Usamos la variable en lugar de un número estático
        leftSidebar.setBounds(area.removeFromLeft(leftSidebarWidth));
        
        // Le asignamos 4 píxeles de ancho a la barra para agarrarla cómodamente
        sidebarResizer.setBounds(area.removeFromLeft(4)); 
    }
    else {
        leftSidebar.setVisible(false);
        sidebarResizer.setVisible(false);
    }

    trackContainer.setBounds(area.removeFromLeft(250));
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