#pragma once
#include <JuceHeader.h>
#include "../Data/LoudnessTrackData.h"
#include "../../../Engine/SimpleLoudness.h"

/**
 * LoudnessTrackDSP — Lógica de grabación de historial en tiempo real.
 * Transfiere los datos desde el Engine (SimpleLoudness) hacia el Data (History).
 */
class LoudnessTrackDSP {
public:
    static void recordAnalysis(LoudnessTrackData& data, 
                               SimpleLoudness& loudness,
                               double currentSamplePos);
};
