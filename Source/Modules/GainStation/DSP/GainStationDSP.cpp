#include "GainStationDSP.h"
#include "../../../Data/Track.h"

void GainStationDSP::processPreFX(GainStationData& data, Track* track, juce::AudioBuffer<float>& buffer) {
    auto snap = data.getSnapshot();

    // 1. APLICAR PRE-GAIN (Input Trim)
    buffer.applyGain(snap.preGain);

    // 2. INVERSIÓN DE POLARIDAD (Phase)
    if (snap.isPhaseInverted) {
        buffer.applyGain(-1.0f);
    }

    // 3. LECTURA PRE-FX (Análisis)
    // Mantenemos la dependencia temporal con el track hasta modularizar el Loudness
    if (track) track->dsp.preLoudness.process(buffer);
}

void GainStationDSP::processPostFX(GainStationData& data, Track* track, juce::AudioBuffer<float>& buffer) {
    auto snap = data.getSnapshot();

    // 1. APLICAR POST-GAIN (Output Compensation)
    buffer.applyGain(snap.postGain);

    // 2. MONO CHECK (Suma de canales a Mono)
    if (snap.isMonoActive && buffer.getNumChannels() >= 2) {
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);
        int numSamples = buffer.getNumSamples();
        for (int i = 0; i < numSamples; ++i) {
            float monoSum = (left[i] + right[i]) * 0.5f;
            left[i] = monoSum;
            right[i] = monoSum;
        }
    }

    // 3. LECTURA POST-FX (Análisis)
    if (track) {
        track->dsp.postLoudness.process(buffer);
        track->dsp.postBalance.process(buffer);
        track->dsp.postMidSide.process(buffer);
    }
}
