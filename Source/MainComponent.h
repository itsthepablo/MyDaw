#pragma once
#include <JuceHeader.h>
#include "GlobalData.h"
#include "Playlist/PlaylistComponent.h" 
#include "Mixer/MixerComponent.h" 
#include "Tracks/TrackContainer.h"
#include "Effects/EffectsPanel.h"
#include "PianoRoll/PianoRollComponent.h" 
#include "Engine/AudioEngine.h" // <--- NUEVO: Incluimos el Motor de Audio

class MainComponent : public juce::AudioAppComponent,
                      public juce::ApplicationCommandTarget
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& c) override;
    void getCommandInfo(juce::CommandID, juce::ApplicationCommandInfo&) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo&) override;

    void togglePlay();

private:
    juce::ApplicationCommandManager commandManager;
    TrackContainer trackContainer;
    PlaylistComponent playlistUI;
    MixerComponent mixerUI;
    EffectsPanel effectsPanelUI;

    // --- Variables del Piano Roll ---
    PianoRollComponent pianoRollUI;
    std::unique_ptr<juce::DocumentWindow> pianoRollWindow;

    juce::TextButton playBtn, showMixerBtn;
    bool isMixerVisible = false;
    bool isEffectsPanelVisible = false;

    // --- El candado de seguridad (Se queda aquí porque lo usa la UI y el Motor) ---
    juce::CriticalSection audioMutex;

    // --- NUESTRO NUEVO MOTOR DE AUDIO ---
    AudioEngine audioEngine;

    // --- Estados y Comandos ---
    const juce::CommandID playStopCommand = 0x1001;
    
    // NOTA: wasPlayingLastBlock, lastPreviewPitch, currentSampleRate y currentBlockSize
    // se han borrado de aquí porque ahora viven dentro de la clase AudioEngine.

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};