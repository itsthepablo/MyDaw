#include "BridgeManager.h"
#include "TrackPianoRollBridge.h"
#include "TrackEffectsBridge.h"
#include "TrackMixerPlaylistBridge.h"
#include "TransportBridge.h" 
#include "InterfaceBridge.h"

void BridgeManager::initializeAllBridges(BridgeDependencies d) {
    TrackPianoRollBridge::connect(d.trackContainer, d.playlistUI, d.pianoRollUI, d.openPianoRoll);
    TrackPianoRollBridge::connectPlaylist(d.playlistUI, d.pianoRollUI, d.openPianoRoll);

    TrackEffectsBridge::connect(d.trackContainer, d.effectsPanelUI, d.audioMutex,
        d.audioEngine.clock.sampleRate, d.audioEngine.clock.blockSize,
        d.switchToArrangementWithEffects);

    TrackMixerPlaylistBridge::connect(d.trackContainer, d.mixerUI, d.playlistUI);
    TransportBridge::connect(d.transportBar, d.topMenuBar, d.pianoRollUI, d.playlistUI);

    InterfaceBridge::connect(d.toolbarButtons, d.isBottomDockVisible, d.isLeftSidebarVisible,
        d.bottomDock, d.effectsPanelUI, d.leftSidebar, d.trackContainer,
        d.onResized, d.onToggleView);
}

void BridgeManager::cleanupTrack(Track* track, PianoRollComponent& pianoRoll, std::function<void()> closePianoRollCallback) {
    TrackPianoRollBridge::cleanup(pianoRoll, closePianoRollCallback, track);
}