#include "SelectionManager.h"

SelectionManager::SelectionManager(DAWUIComponents& uiComponents, AudioEngine& engine)
    : ui(uiComponents), audioEngine(engine)
{
}

void SelectionManager::selectTrack(Track* t, bool isMaster) {
    if (isMaster) {
        ui.trackContainer.deselectAllTracks();
        if (ui.masterTrackObj) ui.masterTrackObj->isSelected = true;
    } else {
        if (ui.masterTrackObj) ui.masterTrackObj->isSelected = false;
    }

    // --- ACTUALIZACIÓN GLOBAL DE PANELES ---
    audioEngine.selectedTrackForAnalysis.store(t);
    ui.rightDock.setTrack(t);
    ui.effectsPanelUI.setTrack(t);
    ui.instrumentPanelUI.setTrack(t);

    ui.masterStrip.repaint();
    ui.trackContainer.repaint();
    ui.mixerUI.repaint();
    ui.miniMixerUI.repaint();
}
