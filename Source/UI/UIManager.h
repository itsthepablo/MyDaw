#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Playlist/PlaylistComponent.h"
#include "../PianoRoll/PianoRollComponent.h"
#include "../Mixer/MixerComponent.h"
#include "../Mixer/MasterChannelUI.h" 
#include "../Effects/EffectsPanel.h"
#include "../Instruments/InstrumentPanel.h"
#include "../UI/TransportBar.h"
#include "../UI/Buttons/ToolbarButtons.h"
#include "../UI/ResourceMeter.h"
#include "../UI/PickerPanel.h"
#include "../UI/FileBrowserPanel.h" 
#include "../UI/ChannelRackPanel.h" 
#include "../UI/LeftSidebar.h"
#include "../UI/BottomDock.h"       
#include "../UI/SidebarResizer.h" 
#include "../UI/BottomDockResizer.h" 
#include "../UI/HintPanel.h"   
#include "TopMenuBar/TopMenuBar.h"

struct DAWUIComponents {
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
    InstrumentPanel instrumentPanelUI;
    BottomDock bottomDock{ rackPanelUI, effectsPanelUI, instrumentPanelUI };
    BottomDockResizer bottomDockResizer;
    PickerPanel pickerPanelUI;
    FileBrowserPanel fileBrowserPanelUI;
    LeftSidebar leftSidebar{ pickerPanelUI, fileBrowserPanelUI };
    SidebarResizer sidebarResizer;
    TransportBar transportBar;
    
    MasterTrackStrip masterStrip;
    std::unique_ptr<Track> masterTrackObj;

    std::unique_ptr<ResourceMeter> resourceMeter;
};

class UIManager {
public:
    static void setupUI(DAWUIComponents& ui, juce::Component& parent, std::function<void()> onClosePianoRoll);
};