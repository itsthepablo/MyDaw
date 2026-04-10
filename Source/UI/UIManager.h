#pragma once
#include <JuceHeader.h>
#include "../Tracks/UI/TrackContainer.h"
#include "../Playlist/PlaylistComponent.h"
#include "../PianoRoll/PianoRollComponent.h"
#include "../Mixer/MixerComponent.h"
// MasterChannelUI eliminado - se usará MixerChannelUI para el Master
#include "Panels/Effects/EffectsPanel.h"
#include "Panels/Instruments/InstrumentPanel.h"
#include "Panels/Browsers/PickerPanel.h"
#include "Panels/Browsers/FileBrowserPanel.h"
#include "Panels/ChannelRack/ChannelRackPanel.h"
#include "Panels/HintPanel.h"
#include "Meters/ResourceMeter.h"
#include "Docks/LeftSidebar/LeftSidebar.h"
#include "Docks/LeftSidebar/SidebarResizer.h"
#include "Docks/BottomDock/BottomDock.h"
#include "Bars/TopMenuBar/TopMenuBar.h"
#include "../UI/Buttons/ToolbarButtons.h"

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
    
    MasterTrackStrip masterStrip;
    std::unique_ptr<Track> masterTrackObj;

    std::unique_ptr<ResourceMeter> resourceMeter;

    std::function<void()> onResized;
};

class UIManager {
public:
    static void setupUI(DAWUIComponents& ui, juce::Component& parent, std::function<void()> onClosePianoRoll);
};