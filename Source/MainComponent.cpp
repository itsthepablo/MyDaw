#include "MainComponent.h"
#include "Bridge/TrackPianoRollBridge.h"
#include "Bridge/TrackEffectsBridge.h"
#include "Bridge/TrackMixerPlaylistBridge.h"
#include "Bridge/TransportBridge.h" 

MainComponent::MainComponent() {
    commandManager.registerAllCommandsForTarget(this);
    addKeyListener(commandManager.getKeyMappings());

    addAndMakeVisible(transportBar);
    addAndMakeVisible(effectsPanelUI);
    addAndMakeVisible(trackContainer);
    addAndMakeVisible(playlistUI);
    addAndMakeVisible(mixerUI);

    // Connexions mitjançant els Bridges
    TrackPianoRollBridge::connect(trackContainer, pianoRollUI, pianoRollWindow);
    TrackEffectsBridge::connect(trackContainer, effectsPanelUI, audioMutex,
        audioEngine.clock.sampleRate, audioEngine.clock.blockSize,
        isEffectsPanelVisible, [this] { resized(); });
    TrackMixerPlaylistBridge::connect(trackContainer, mixerUI, playlistUI);
    TransportBridge::connect(transportBar, pianoRollUI, playlistUI);

    trackContainer.onDeleteTrack = [this](int index) {
        const juce::ScopedLock sl(audioMutex);
        if (index >= 0 && index < trackContainer.getTracks().size()) {
            TrackPianoRollBridge::cleanup(pianoRollUI, pianoRollWindow, trackContainer.getTracks()[index]);
            trackContainer.removeTrack(index);

            // Actualitzem la playlist perquè el scroll es recalculi immediatament
            playlistUI.updateScrollBars();
            playlistUI.repaint();

            isEffectsPanelVisible = false;
            resized();
        }
        };

    // Configuració del botó del Mixer
    addAndMakeVisible(showMixerBtn);
    showMixerBtn.setButtonText("MIXER");
    showMixerBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    showMixerBtn.onClick = [this] {
        isMixerVisible = !isMixerVisible;
        mixerUI.setVisible(isMixerVisible); // Sincronitzem visibilitat
        resized();
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

    // Ubicació de la barra superior i el botó del Mixer
    auto topArea = area.removeFromTop(45);
    showMixerBtn.setBounds(topArea.removeFromRight(80).reduced(5));
    transportBar.setBounds(topArea);

    // Lògica de visibilitat del Mixer
    if (isMixerVisible) {
        mixerUI.setVisible(true);
        mixerUI.setBounds(area.removeFromBottom(getHeight() * 0.35));
    }
    else {
        mixerUI.setVisible(false);
    }

    // Lògica de visibilitat del panell d'efectes
    if (isEffectsPanelVisible) {
        effectsPanelUI.setVisible(true);
        effectsPanelUI.setBounds(area.removeFromLeft(220));
    }
    else {
        effectsPanelUI.setVisible(false);
    }

    // El que queda es reparteix entre tracks i playlist
    trackContainer.setBounds(area.removeFromLeft(250));
    playlistUI.setBounds(area);
}

// Implementació de comandos per al teclat (Espai per a Play/Stop)
juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget() { return nullptr; }

void MainComponent::getAllCommands(juce::Array<juce::CommandID>& c) {
    c.add(playStopCommand);
}

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