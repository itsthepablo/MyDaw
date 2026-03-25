#pragma once
#include <JuceHeader.h>
#include "Tracks/TrackContainer.h"
#include "Playlist/PlaylistComponent.h"
#include "PianoRoll/PianoRollComponent.h"
#include "Mixer/MixerComponent.h"
#include "Mixer/MasterChannelUI.h" 
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
#include "Project/ProjectManager.h"
#include "Bridge/BridgeManager.h"
#include "UI/Layout/LayoutHandler.h"
#include "UI/Commands/DAWCommandHandler.h" // Nuevo

class MainComponent : public juce::AudioAppComponent,
    public juce::ApplicationCommandTarget, // Seguimos siendo target para enlazar el manager
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

    // Métodos de CommandTarget (ahora delegan al handler)
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& c) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) override;

    // keyPressed ya no es necesario si usamos Comandos para el Tab
    void toggleViewMode();
    void loadProject(const juce::File& file);

private:
    void setupUIComponents();
    void setupCallbacks();
    void setupBridges();
    void setupCommands(); // Nuevo método

    void openPianoRoll();
    void closePianoRoll();
    void saveProject();

    juce::ApplicationCommandManager commandManager;
    std::unique_ptr<DAWCommandHandler> commandHandler; // Manejador real

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

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};