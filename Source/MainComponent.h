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
#include "UI/FileBrowserPanel.h" 
#include "UI/ChannelRackPanel.h" 
#include "UI/LeftSidebar.h"
#include "UI/BottomDock.h"       
#include "UI/SidebarResizer.h" 
#include "UI/BottomDockResizer.h" 
#include "UI/HintPanel.h"   
#include "UI/TopMenuBar.h" 
#include "Engine/AudioEngine.h"

enum class ViewMode { Arrangement, Mixer };

class MainComponent : public juce::AudioAppComponent,
    public juce::ApplicationCommandTarget,
    public juce::DragAndDropContainer
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

    bool keyPressed(const juce::KeyPress& key) override;
    void toggleViewMode();

private:
    void openPianoRoll();
    void closePianoRoll();

    juce::ApplicationCommandManager commandManager;
    const juce::CommandID playStopCommand = 1;

    ViewMode currentView = ViewMode::Arrangement;

    TopMenuBar topMenuBar;
    HintPanel hintPanel;

    TrackContainer trackContainer;
    PlaylistComponent playlistUI;

    PianoRollComponent pianoRollUI;
    juce::TextButton closePianoRollBtn;

    MixerComponent mixerUI;
    MasterChannelUI masterChannelUI{ mixerUI };
    ChannelRackPanel rackPanelUI;
    EffectsPanel effectsPanelUI;
    BottomDock bottomDock{ rackPanelUI, effectsPanelUI };
    BottomDockResizer bottomDockResizer;

    PickerPanel pickerPanelUI;
    FileBrowserPanel fileBrowserPanelUI;
    LeftSidebar leftSidebar{ pickerPanelUI, fileBrowserPanelUI };
    SidebarResizer sidebarResizer;

    TransportBar transportBar;
    ToolbarButtons toolbarButtons;
    std::unique_ptr<ResourceMeter> resourceMeter;

    juce::CriticalSection audioMutex;
    AudioEngine audioEngine;

    bool isBottomDockVisible = true;
    int bottomDockHeight = 250;
    bool isLeftSidebarVisible = true;
    int leftSidebarWidth = 200;

    bool isPianoRollVisible = false;
    bool prePianoRollLeftSidebar = true;
    bool prePianoRollBottomDock = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};