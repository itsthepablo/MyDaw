#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Playlist/PlaylistComponent.h"
#include "../PianoRoll/PianoRollComponent.h"
#include "../Mixer/MixerComponent.h"
// MasterChannelUI eliminado - se usará MixerChannelUI para el Master
#include "Panels/Effects/EffectsPanel.h"
#include "Panels/Instruments/InstrumentPanel.h"
#include "../UI/TransportBar.h"
#include "../UI/Buttons/ToolbarButtons.h"
#include "../UI/ResourceMeter.h"
#include "Panels/Browsers/PickerPanel.h"
#include "Panels/Browsers/FileBrowserPanel.h"
#include "Panels/ChannelRack/ChannelRackPanel.h"
#include "../UI/LeftSidebar.h"
#include "../UI/BottomDock.h"       
#include "../UI/SidebarResizer.h" 
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
    MixerComponent miniMixerUI;
    
    // Master Channel ahora es un MixerChannelUI completo
    std::unique_ptr<MixerChannelUI> masterChannelUI;

    ChannelRackPanel rackPanelUI;
    EffectsPanel effectsPanelUI;
    InstrumentPanel instrumentPanelUI;
    BottomDock bottomDock{ rackPanelUI, effectsPanelUI, instrumentPanelUI, miniMixerUI };
    PickerPanel pickerPanelUI;
    FileBrowserPanel fileBrowserPanelUI;
    LeftSidebar leftSidebar{ pickerPanelUI, fileBrowserPanelUI };
    SidebarResizer sidebarResizer;
    TransportBar transportBar;
    
    MasterTrackStrip masterStrip;
    std::unique_ptr<Track> masterTrackObj;

    std::unique_ptr<ResourceMeter> resourceMeter;

    std::function<void()> onResized;
};

class UIManager {
public:
    static void setupUI(DAWUIComponents& ui, juce::Component& parent, std::function<void()> onClosePianoRoll);
};