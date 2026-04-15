#pragma once
#include <JuceHeader.h>
#include "../../Data/Track.h"

/**
 * MixerDSP: Motor de procesamiento profesional.
 * Utiliza el NativeModulationManager para aplicar modulación universal (LEARN)
 * de forma suave y sin artefactos.
 */
class MixerDSP {
public:
    static void applyGainAndPan(Track* track, int numSamples, int hardwareOutChannels, double phaseStart, double phaseEnd) noexcept;
};
