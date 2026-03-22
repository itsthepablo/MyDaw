#pragma once
#include <JuceHeader.h>
#include "GlobalData.h"
#include "Playlist/PlaylistComponent.h" 
#include "Mixer/MixerComponent.h" 
#include "Tracks/TrackContainer.h"
#include "Effects/EffectsPanel.h"
#include "PianoRoll/PianoRollComponent.h" 
#include "Engine/AudioEngine.h"
#include "UI/TransportBar.h"
#include "UI/Buttons/ToolbarButtons.h"
#include "UI/ResourceMeter.h" // <--- NUEVO MEDIDOR

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
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) override;

private:
    juce::ApplicationCommandManager commandManager;
    TrackContainer trackContainer;
    PlaylistComponent playlistUI;
    MixerComponent mixerUI;
    EffectsPanel effectsPanelUI;
    PianoRollComponent pianoRollUI;

    std::unique_ptr<juce::DocumentWindow> pianoRollWindow;
    TransportBar transportBar;
    ToolbarButtons toolbarButtons; 
    
    std::unique_ptr<ResourceMeter> resourceMeter; // <--- PUNTERO AL MEDIDOR

    bool isMixerVisible = false;
    bool isEffectsPanelVisible = false;

    juce::CriticalSection audioMutex;
    AudioEngine audioEngine;

    const juce::CommandID playStopCommand = 0x1001;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};