#include "TrackManager.h"
#include "../Bridge/BridgeManager.h"

TrackManager::TrackManager(DAWUIComponents& uiComponents, AudioEngine& engine, juce::CriticalSection& mutex)
    : ui(uiComponents), audioEngine(engine), audioMutex(mutex)
{
}

void TrackManager::setup() {
    // Aquí podríamos conectar más cosas si fuera necesario en el futuro
}

void TrackManager::prepareTracks(double sampleRate, int samplesPerBlock) {
    // Alocar recursos estables para pistas cargadas
    for (auto* t : ui.trackContainer.getTracks()) {
        prepareSingleTrack(t, sampleRate, samplesPerBlock);
    }

    // Preparar Master Track
    if (ui.masterTrackObj) {
        prepareSingleTrack(ui.masterTrackObj.get(), sampleRate, samplesPerBlock);
        audioEngine.setMasterTrack(ui.masterTrackObj.get());
    }

    // Primera carga topológica
    audioEngine.routingMatrix.commitNewTopology(ui.trackContainer.getTracks());
}

void TrackManager::prepareSingleTrack(Track* t, double sampleRate, int samplesPerBlock) {
    if (!t) return;

    // MAX SIZE estricto para evitar bounds overwrite failures
    int maxSamples = 8192;
    t->audioBuffer.setSize(2, maxSamples, false, true, false);
    t->instrumentMixBuffer.setSize(2, maxSamples, false, true, false);
    t->tempSynthBuffer.setSize(2, maxSamples, false, true, false);
    t->midSideBuffer.setSize(2, maxSamples, false, true, false);
    t->audioBuffer.clear();

    // Sincronizar buffers de PDC si existen plugins
    if (!t->plugins.isEmpty()) {
        t->dsp.pdcBuffer.clear();
    }

    // Preparar el motor y publicar snapshot inicial
    t->prepare(sampleRate, samplesPerBlock);
    t->commitSnapshot();
}

void TrackManager::handleToggleAnalysisTrack(TrackType type, bool visible) {
    Track* targetTrack = nullptr;
    int trackIndex = -1;
    auto& tracks = ui.trackContainer.getTracks();
    
    for (int i = 0; i < tracks.size(); ++i) {
        if (tracks[i]->getType() == type) {
            targetTrack = tracks[i];
            trackIndex = i;
            break;
        }
    }

    if (visible) {
        if (targetTrack == nullptr) {
            targetTrack = ui.trackContainer.addTrack(type);
            double currentSR = audioEngine.currentSampleRate > 0 ? audioEngine.currentSampleRate : 44100.0;
            
            // Configurar DSP específico del tipo de pista de análisis
            targetTrack->dsp.postLoudness.prepare(currentSR, 512);
            targetTrack->dsp.postBalance.prepare(currentSR, 512);
            targetTrack->dsp.postMidSide.prepare(currentSR);
            
            prepareSingleTrack(targetTrack, currentSR, 4096);
            
            if (type == TrackType::Loudness) { targetTrack->setColor(juce::Colours::orange); targetTrack->setName("Loudness Track"); }
            else if (type == TrackType::Balance) { targetTrack->setColor(juce::Colours::cyan); targetTrack->setName("Balance Track"); }
            else if (type == TrackType::MidSide) { targetTrack->setColor(juce::Colours::magenta); targetTrack->setName("Mid-Side Track"); }
        }
        targetTrack->isShowingInChildren = true;
    } else {
        if (trackIndex != -1) {
            // Eliminar invocando el callback de borrado (que ahora llegará a handleDeleteTrack)
            ui.trackContainer.onDeleteTrack(trackIndex);
        }
    }
    
    ui.trackContainer.recalculateFolderHierarchy();
    ui.trackContainer.resized();
    ui.trackContainer.repaint();
    ui.playlistUI.updateScrollBars();
    ui.playlistUI.repaint();
}

void TrackManager::handleDeleteTrack(int index, std::function<void()> onPianoRollCleanup) {
    const juce::ScopedLock sl(audioMutex);
    
    if (index >= 0 && index < ui.trackContainer.getTracks().size()) {
        Track* trackToDelete = ui.trackContainer.getTracks()[index];
        
        // Limpieza de puentes y Piano Roll
        BridgeManager::cleanupTrack(trackToDelete, ui.pianoRollUI, onPianoRollCleanup);

        // Sincronizar estados de switches en UI si es una pista de análisis
        if (trackToDelete->getType() == TrackType::Loudness) ui.trackContainer.setAnalysisTrackToggleState(TrackType::Loudness, false);
        else if (trackToDelete->getType() == TrackType::Balance) ui.trackContainer.setAnalysisTrackToggleState(TrackType::Balance, false);
        else if (trackToDelete->getType() == TrackType::MidSide) ui.trackContainer.setAnalysisTrackToggleState(TrackType::MidSide, false);

        ui.playlistUI.purgeClipsOfTrack(trackToDelete);
        ui.trackContainer.removeTrack(index); 
        
        audioEngine.routingMatrix.commitNewTopology(ui.trackContainer.getTracks());

        ui.playlistUI.updateScrollBars();
        ui.playlistUI.repaint();
        ui.pickerPanelUI.refreshList();

        // Limpiar track seleccionado en paneles si era este
        if (ui.leftSidebar.getCurrentTab() == LeftSidebar::EffectsTab)
            ui.effectsPanelUI.setTrack(nullptr);
        if (ui.bottomDock.getCurrentTab() == BottomDock::InstrumentTab)
            ui.instrumentPanelUI.setTrack(nullptr);

        ui.onResized(); // Disparar layout general
    }
}

void TrackManager::handleTrackAdded(Track* newTrack) {
    if (newTrack) {
        double currentSR = audioEngine.currentSampleRate > 0 ? audioEngine.currentSampleRate : 44100.0;
        prepareSingleTrack(newTrack, currentSR, 4096);
    }
    
    audioEngine.routingMatrix.commitNewTopology(ui.trackContainer.getTracks());
    
    ui.playlistUI.updateScrollBars();
    ui.playlistUI.repaint();
}

void TrackManager::handleTracksReordered() {
    audioEngine.routingMatrix.commitNewTopology(ui.trackContainer.getTracks());
    ui.playlistUI.updateScrollBars();
    ui.playlistUI.repaint();
}
