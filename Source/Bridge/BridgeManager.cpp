#include "BridgeManager.h"
#include "TrackPianoRollBridge.h"
#include "../UI/Panels/Effects/Bridges/TrackEffectsBridge.h"
#include "TrackMixerPlaylistBridge.h"
#include "TransportBridge.h" 
#include "InterfaceBridge.h"
#include "../UI/Panels/Instruments/InstrumentPanel.h"
#include "../UI/Panels/Instruments/Bridges/TrackInstrumentBridge.h"

void BridgeManager::initializeAllBridges(BridgeDependencies d) {
    TrackPianoRollBridge::connect(d.trackContainer, d.playlistUI, d.pianoRollUI, d.openPianoRoll);
    TrackPianoRollBridge::connectPlaylist(d.playlistUI, d.pianoRollUI, d.openPianoRoll);

    TrackEffectsBridge::connect(d.trackContainer, d.effectsPanelUI, d.mixerUI, d.miniMixerUI, d.masterChannelUI, d.audioMutex,
        d.audioEngine.clock.sampleRate, d.audioEngine.clock.maxBlockSize,
        d.audioEngine, 
        d.switchToArrangementWithEffects,
        &d.playlistUI);

    TrackMixerPlaylistBridge::connect(d.trackContainer, d.mixerUI, d.miniMixerUI, d.playlistUI, d.masterTrack);
    TransportBridge::connect(d.transportBar, d.topMenuBar, d.pianoRollUI, d.playlistUI);

    InterfaceBridge::connect(d.topMenuBar, d.isBottomDockVisible, d.isLeftSidebarVisible,
        d.bottomDock, d.effectsPanelUI, d.leftSidebar, d.trackContainer,
        d.onResized, d.onToggleView);

    TrackInstrumentBridge::connect(
        d.trackContainer,
        d.instrumentPanelUI,
        d.effectsPanelUI,
        d.bottomDock,
        d.isBottomDockVisible,
        d.audioMutex,
        d.audioEngine,
        d.onResized
    );

    // ---> NUEVO: CONEXIÓN GRÁFICA A 60 FPS (Zero-Allocation) <---
    auto& engine = d.audioEngine; // Captura segura del motor
    d.playlistUI.getPlaybackPosition = [&engine]() -> float { return engine.clock.currentPh; };
    d.pianoRollUI.getPlaybackPosition = [&engine]() -> float { return engine.clock.currentPh; };

    // --- NUEVO: Conexión asíncrona hacia el AudioEngine (Lock-Free Seek) ---
    d.playlistUI.onPlayheadSeekRequested = [&engine](float newPos) {
        engine.transportState.seekRequestPh.store(newPos, std::memory_order_release);
        engine.transportState.playheadPos.store(newPos, std::memory_order_release);
    };
}

void BridgeManager::cleanupTrack(Track* track, PianoRollComponent& pianoRoll, std::function<void()> closePianoRollCallback) {
    TrackPianoRollBridge::cleanup(pianoRoll, closePianoRollCallback, track);
}