#include "MainComponent.h"
#include "Bridge/TrackPianoRollBridge.h"
#include "Bridge/TrackEffectsBridge.h"
#include "Bridge/TrackMixerPlaylistBridge.h"
#include "Bridge/TransportBridge.h" 
#include "Bridge/InterfaceBridge.h"

MainComponent::MainComponent() {
    commandManager.registerAllCommandsForTarget(this);
    addKeyListener(commandManager.getKeyMappings());

    // Inicializamos el medidor pasándole una referencia a la aplicación de audio
    resourceMeter = std::make_unique<ResourceMeter>(*this);

    addAndMakeVisible(transportBar);
    addAndMakeVisible(toolbarButtons);
    addAndMakeVisible(*resourceMeter); // <--- HACEMOS VISIBLE EL MEDIDOR
    addAndMakeVisible(effectsPanelUI);
    addAndMakeVisible(trackContainer);
    addAndMakeVisible(playlistUI);
    addAndMakeVisible(mixerUI);

    mixerUI.setVisible(false);
    effectsPanelUI.setVisible(false);

    // Protección de memoria (Mantenido de la versión segura)
    trackContainer.setExternalMutex(&audioMutex);

    // --- CONEXIONES MEDIANTE BRIDGES ---
    TrackPianoRollBridge::connect(trackContainer, pianoRollUI, pianoRollWindow);

    TrackEffectsBridge::connect(trackContainer, effectsPanelUI, audioMutex,
        audioEngine.clock.sampleRate, audioEngine.clock.blockSize,
        isEffectsPanelVisible, [this] { resized(); });

    TrackMixerPlaylistBridge::connect(trackContainer, mixerUI, playlistUI);
    TransportBridge::connect(transportBar, pianoRollUI, playlistUI);

    InterfaceBridge::connect(toolbarButtons, isMixerVisible, isEffectsPanelVisible,
        mixerUI, effectsPanelUI, trackContainer, [this] { resized(); });

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
    toolbarButtons.setBounds(topArea.removeFromRight(180));
    
    // UBICACIÓN DEL MEDIDOR: A la derecha, al lado de los botones de la barra de herramientas
    resourceMeter->setBounds(topArea.removeFromRight(100).reduced(8, 10)); 
    
    transportBar.setBounds(topArea);

    if (isMixerVisible) {
        mixerUI.setVisible(true);
        mixerUI.setBounds(area.removeFromBottom(getHeight() * 0.35));
    }
    else {
        mixerUI.setVisible(false);
    }

    if (isEffectsPanelVisible) {
        effectsPanelUI.setVisible(true);
        effectsPanelUI.setBounds(area.removeFromLeft(220));
    }
    else {
        effectsPanelUI.setVisible(false);
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