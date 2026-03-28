#include "BridgeManager.h"
#include "TrackPianoRollBridge.h"
#include "TrackEffectsBridge.h"
#include "TrackMixerPlaylistBridge.h"
#include "TransportBridge.h" 
#include "InterfaceBridge.h"
#include "TrackInstrumentBridge.h"

void BridgeManager::initializeAllBridges(BridgeDependencies d) {
    TrackPianoRollBridge::connect(d.trackContainer, d.playlistUI, d.pianoRollUI, d.openPianoRoll);
    TrackPianoRollBridge::connectPlaylist(d.playlistUI, d.pianoRollUI, d.openPianoRoll);

    TrackEffectsBridge::connect(d.trackContainer, d.effectsPanelUI, d.audioMutex,
        d.audioEngine.clock.sampleRate, d.audioEngine.clock.blockSize,
        d.switchToArrangementWithEffects);

    TrackMixerPlaylistBridge::connect(d.trackContainer, d.mixerUI, d.playlistUI);
    TransportBridge::connect(d.transportBar, d.topMenuBar, d.pianoRollUI, d.playlistUI);

    InterfaceBridge::connect(d.toolbarButtons, d.topMenuBar, d.isBottomDockVisible, d.isLeftSidebarVisible,
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
}

void BridgeManager::cleanupTrack(Track* track, PianoRollComponent& pianoRoll, std::function<void()> closePianoRollCallback) {
    TrackPianoRollBridge::cleanup(pianoRoll, closePianoRollCallback, track);
}