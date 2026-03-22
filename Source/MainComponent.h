#pragma once
#include <JuceHeader.h>
#include "Tracks/TrackContainer.h"
#include "Playlist/PlaylistComponent.h"
#include "PianoRoll/PianoRollComponent.h"
#include "Mixer/MixerComponent.h"
#include "Effects/EffectsPanel.h"
#include "UI/TransportBar.h"
#include "UI/Buttons/ToolbarButtons.h"
#include "UI/ResourceMeter.h"
#include "UI/PickerPanel.h"
#include "UI/LeftSidebar.h"
#include "UI/SidebarResizer.h" // NUEVO INCLUDE
#include "Engine/AudioEngine.h"

class MainComponent : public juce::AudioAppComponent, public juce::ApplicationCommandTarget {
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
    const juce::CommandID playStopCommand = 1;

    TrackContainer trackContainer;
    PlaylistComponent playlistUI;
    PianoRollComponent pianoRollUI;
    MixerComponent mixerUI;
    
    EffectsPanel effectsPanelUI;
    PickerPanel pickerPanelUI; 
    
    LeftSidebar leftSidebar{pickerPanelUI, effectsPanelUI}; 
    SidebarResizer sidebarResizer; // NUEVA BARRA DIVISORIA

    TransportBar transportBar;
    ToolbarButtons toolbarButtons;
    std::unique_ptr<ResourceMeter> resourceMeter;
    
    std::unique_ptr<juce::DocumentWindow> pianoRollWindow;

    juce::CriticalSection audioMutex;
    AudioEngine audioEngine;

    bool isMixerVisible = false;
    bool isLeftSidebarVisible = true; 
    int leftSidebarWidth = 200; // NUEVA MEMORIA DEL ANCHO DEL PANEL
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};