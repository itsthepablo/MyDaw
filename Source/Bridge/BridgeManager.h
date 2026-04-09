#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Playlist/PlaylistComponent.h"
#include "../PianoRoll/PianoRollComponent.h"
#include "../Mixer/MixerComponent.h"
#include "../UI/Panels/Effects/EffectsPanel.h"
#include "../UI/Panels/Instruments/InstrumentPanel.h"
#include "../UI/TransportBar.h"
#include "../UI/TopMenuBar/TopMenuBar.h"
#include "../UI/BottomDock.h"
#include "../UI/LeftSidebar.h"
#include "../Engine/Core/AudioEngine.h"

struct BridgeDependencies {
    TrackContainer& trackContainer;
    PlaylistComponent& playlistUI;
    PianoRollComponent& pianoRollUI;
    MixerComponent& mixerUI;
    MixerComponent& miniMixerUI;
    MixerChannelUI* masterChannelUI; // NUEVO - El Master Channel complejo
    EffectsPanel& effectsPanelUI;
    InstrumentPanel& instrumentPanelUI; // <-- NUEVO
    TransportBar& transportBar;
    TopMenuBar& topMenuBar;
    BottomDock& bottomDock;
    LeftSidebar& leftSidebar;
    AudioEngine& audioEngine;
    juce::CriticalSection& audioMutex;

    bool& isBottomDockVisible;
    bool& isLeftSidebarVisible;

    std::function<void()> openPianoRoll;
    std::function<void()> closePianoRoll;
    std::function<void()> onResized;
    std::function<void()> onToggleView;
    std::function<void()> switchToArrangementWithEffects;

    Track* masterTrack;
};

class BridgeManager {
public:
    static void initializeAllBridges(BridgeDependencies d);

    // Nuevo método para limpiar un track de los sistemas de Piano Roll
    static void cleanupTrack(Track* track, PianoRollComponent& pianoRoll, std::function<void()> closePianoRollCallback);
};