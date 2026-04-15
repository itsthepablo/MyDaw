#include "GainStationDSP.h"
#include "../../../Data/Track.h"

void GainStationDSP::processPreFX(GainStationData& data, Track* track, juce::AudioBuffer<float>& buffer) {
    auto snap = data.getSnapshot();
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // 1. APLICAR PRE-GAIN (Input Trim + Modulación)
    for (int i = 0; i < numSamples; ++i) {
        // Modulación aditiva (+/- 1.0) sobre el gain base
        float mod = data.modPreGainSmoother.getNextValue();
        float totalG = juce::jlimit(0.0f, 4.0f, snap.preGain + mod);
        
        for (int ch = 0; ch < numChannels; ++ch) {
            buffer.setSample(ch, i, buffer.getSample(ch, i) * totalG);
        }
    }

    // 2. INVERSIÓN DE POLARIDAD (Phase)
    if (snap.isPhaseInverted) {
        buffer.applyGain(-1.0f);
    }

    // 3. LECTURA PRE-FX (Análisis)
    if (track) track->dsp.preLoudness.process(buffer);
}

void GainStationDSP::processPostFX(GainStationData& data, Track* track, juce::AudioBuffer<float>& buffer) {
    auto snap = data.getSnapshot();
    int numSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    // 1. APLICAR POST-GAIN (Output Compensation + Modulación)
    for (int i = 0; i < numSamples; ++i) {
        float mod = data.modPostGainSmoother.getNextValue();
        float totalG = juce::jlimit(0.0f, 4.0f, snap.postGain + mod);
        
        for (int ch = 0; ch < numChannels; ++ch) {
            buffer.setSample(ch, i, buffer.getSample(ch, i) * totalG);
        }
    }

    // 2. MONO CHECK (Suma de canales a Mono)
    if (snap.isMonoActive && numChannels >= 2) {
        auto* left = buffer.getWritePointer(0);
        auto* right = buffer.getWritePointer(1);
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
