#pragma once
#include <JuceHeader.h>
#include "../../Tracks/UI/TrackContainer.h"
#include "../../Playlist/PlaylistComponent.h"
#include "../../PianoRoll/PianoRollComponent.h"
#include "../../Mixer/MixerComponent.h"
#include "../Docks/BottomDock/BottomDock.h"
#include "../Docks/LeftSidebar/LeftSidebar.h"
#include "../Docks/LeftSidebar/SidebarResizer.h"
#include "../Bars/TopMenuBar/TopMenuBar.h"
#include "../Panels/HintPanel.h"
#include "../Meters/ResourceMeter.h"

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
    ResourceMeter* resourceMeter; // Puntero porque es unique_ptr
    
    PianoRollComponent& pianoRollUI;
    juce::TextButton& closePianoRollBtn;
    
    MixerComponent& mixerUI;
    MixerChannelUI& masterChannelUI;
    
    TrackContainer& trackContainer;
    PlaylistComponent& playlistUI;
    
    BottomDock& bottomDock;
    LeftSidebar& leftSidebar;
    SidebarResizer& sidebarResizer;
    MasterTrackStrip& masterStrip;

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