#include "MainComponent.h"
#include "Bridge/TrackPianoRollBridge.h"
#include "Bridge/TrackEffectsBridge.h"
#include "Bridge/TrackMixerPlaylistBridge.h"

MainComponent::MainComponent() {
    commandManager.registerAllCommandsForTarget(this); 
    addKeyListener(commandManager.getKeyMappings());
    
    addAndMakeVisible(effectsPanelUI); 
    addAndMakeVisible(trackContainer); 
    addAndMakeVisible(playlistUI); 
    addAndMakeVisible(mixerUI);

    // --- CONEXIONES UI ---
    TrackPianoRollBridge::connect(trackContainer, pianoRollUI, pianoRollWindow);

    TrackEffectsBridge::connect(trackContainer, 
                                effectsPanelUI, 
                                audioMutex, 
        audioEngine.clock.sampleRate, // Ahora usa el del motor
        audioEngine.clock.blockSize,  // Ahora usa el del motor
                                isEffectsPanelVisible, 
                                [this] { resized(); });

    TrackMixerPlaylistBridge::connect(trackContainer, mixerUI, playlistUI);

    // --- BORRAR TRACK ---
    trackContainer.onDeleteTrack = [this](int index) {
        const juce::ScopedLock sl(audioMutex);
        if (index >= 0 && index < trackContainer.getTracks().size()) {
            auto* trackToDelete = trackContainer.getTracks()[index];
            TrackPianoRollBridge::cleanup(pianoRollUI, pianoRollWindow, trackToDelete);

            trackContainer.removeTrack(index); 
            playlistUI.repaint(); 
            isEffectsPanelVisible = false; 
            resized();
        }
    };

    addAndMakeVisible(playBtn); 
    playBtn.setButtonText("PLAY"); 
    playBtn.onClick = [this] { togglePlay(); };

    addAndMakeVisible(showMixerBtn); 
    showMixerBtn.setButtonText("MIXER");
    showMixerBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(50, 55, 60));
    showMixerBtn.onClick = [this] { isMixerVisible = !isMixerVisible; resized(); };

    setSize(1000, 700); 
    setAudioChannels(0, 2);
}

MainComponent::~MainComponent() { shutdownAudio(); }

// --- COMANDOS Y TECLADO ---
void MainComponent::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& result) {
    if (id == playStopCommand) { 
        result.setInfo("Play / Stop", "Transporte", "Transport", 0); 
        result.addDefaultKeypress(' ', 0); 
    }
}
juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget() { return nullptr; }
void MainComponent::getAllCommands(juce::Array<juce::CommandID>& c) { c.add(playStopCommand); }
bool MainComponent::perform(const juce::ApplicationCommandTarget::InvocationInfo& i) { 
    if (i.commandID == playStopCommand) { togglePlay(); return true; } 
    return false; 
}

// ==============================================================================
// MOTOR DE AUDIO (¡Magia pura!)
// ==============================================================================
void MainComponent::prepareToPlay(int samples, double s) {
    audioEngine.prepareToPlay(samples, s, trackContainer, audioMutex);
}

void MainComponent::releaseResources() {}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
    audioEngine.processBlock(bufferToFill, trackContainer, pianoRollUI, playlistUI, mixerUI, audioMutex);
}
// ==============================================================================

void MainComponent::togglePlay() {
    bool newState = !pianoRollUI.getIsPlaying();
    pianoRollUI.setPlaying(newState);
    playlistUI.isPlaying = newState;
    if (newState) { pianoRollUI.setPlayheadPos(0); playlistUI.setPlayheadPos(0); }
    playBtn.setButtonText(newState ? "STOP" : "PLAY");
}

void MainComponent::paint(juce::Graphics& g) { g.fillAll(juce::Colour(30, 32, 35)); }

void MainComponent::resized() {
    auto area = getLocalBounds();
    auto topBar = area.removeFromTop(40);
    playBtn.setBounds(topBar.removeFromLeft(100).reduced(2));
    showMixerBtn.setBounds(topBar.removeFromLeft(100).reduced(2));

    if (isMixerVisible) mixerUI.setBounds(area.removeFromBottom(getHeight() * 0.35)); else mixerUI.setBounds(0, 0, 0, 0);
    if (isEffectsPanelVisible) effectsPanelUI.setBounds(area.removeFromLeft(220)); else effectsPanelUI.setBounds(0, 0, 0, 0);

    trackContainer.setBounds(area.removeFromLeft(250)); playlistUI.setBounds(area);
}