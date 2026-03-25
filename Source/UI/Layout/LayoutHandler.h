#pragma once
#include <JuceHeader.h>
#include "../../Tracks/TrackContainer.h"
#include "../../Playlist/PlaylistComponent.h"
#include "../../PianoRoll/PianoRollComponent.h"
#include "../../Mixer/MixerComponent.h"
#include "../../Mixer/MasterChannelUI.h"
#include "../../UI/BottomDock.h"
#include "../../UI/BottomDockResizer.h"
#include "../../UI/LeftSidebar.h"
#include "../../UI/SidebarResizer.h"
#include "../../UI/TopMenuBar.h"
#include "../../UI/HintPanel.h"
#include "../../UI/TransportBar.h"
#include "../../UI/Buttons/ToolbarButtons.h"
#include "../../UI/ResourceMeter.h"

// Definimos ViewMode aquí o lo importamos si es accesible globalmente
#ifndef VIEWMODE_DEFINED
#define VIEWMODE_DEFINED
enum class ViewMode { Arrangement, Mixer };
#endif

struct LayoutDependencies {
    juce::Rectangle<int> area;
    
    // Componentes
    TopMenuBar& topMenuBar;
    HintPanel& hintPanel;
    ToolbarButtons& toolbarButtons;
    ResourceMeter* resourceMeter; // Puntero porque es unique_ptr
    TransportBar& transportBar;
    
    PianoRollComponent& pianoRollUI;
    juce::TextButton& closePianoRollBtn;
    
    MixerComponent& mixerUI;
    MasterChannelUI& masterChannelUI;
    
    TrackContainer& trackContainer;
    PlaylistComponent& playlistUI;
    
    BottomDock& bottomDock;
    BottomDockResizer& bottomDockResizer;
    LeftSidebar& leftSidebar;
    SidebarResizer& sidebarResizer;

    // Estados
    ViewMode currentView;
    bool isPianoRollVisible;
    bool isBottomDockVisible;
    int bottomDockHeight;
    bool isLeftSidebarVisible;
    int leftSidebarWidth;
};

class LayoutHandler {
public:
    static void performLayout(LayoutDependencies d);
};