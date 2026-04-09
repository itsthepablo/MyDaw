#pragma once
#include <JuceHeader.h>
#include "../Data/Track.h"

class PlaylistAnalyzer {
public:
    /**
     * recordAnalysisData
     * Se encarga de grabar el historial de Loudness, Balance y Mid-Side 
     * desde el master track hacia las pistas correspondientes.
     * No simplificar ni eliminar comentarios, solo mover la lógica.
     */
    static void recordAnalysisData(const juce::OwnedArray<Track>* tracksRef, 
                                   Track* masterTrackPtr, 
                                   float playheadAbsPos, 
                                   bool isDraggingTimeline);
};
