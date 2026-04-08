#pragma once
#include <JuceHeader.h>
#include "../../Tracks/Track.h"
#include "../../Mixer/Bridges/MixerParameterBridge.h"

class GainStationDSP {
public:
    static void processPreFX(Track* track, juce::AudioBuffer<float>& buffer) {
        // 1. APLICAR PRE-GAIN (Input Trim)
        buffer.applyGain(track->getPreGain());

        // 2. INVERSIÓN DE POLARIDAD (Phase)
        if (MixerParameterBridge::isPhaseInverted(track)) {
            buffer.applyGain(-1.0f);
        }

        // 3. LECTURA PRE-FX
        track->preLoudness.process(buffer);
    }

    static void processPostFX(Track* track, juce::AudioBuffer<float>& buffer) {
        // 1. APLICAR POST-GAIN (Output Compensation)
        buffer.applyGain(track->getPostGain());

        // 2. MONO CHECK (Suma de canales a Mono)
        if (track->isMonoActive && buffer.getNumChannels() >= 2) {
            auto* left = buffer.getWritePointer(0);
            auto* right = buffer.getWritePointer(1);
            int numSamples = buffer.getNumSamples();
            for (int i = 0; i < numSamples; ++i) {
                float monoSum = (left[i] + right[i]) * 0.5f;
                left[i] = monoSum;
                right[i] = monoSum;
            }
        }

        // 3. LECTURA POST-FX
        track->postLoudness.process(buffer);
        track->postBalance.process(buffer);
        track->postMidSide.process(buffer);
    }
};