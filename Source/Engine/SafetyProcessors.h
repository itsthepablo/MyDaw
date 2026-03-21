#pragma once
#include <JuceHeader.h>
#include <cmath>

class SafetyProcessors {
public:
    // Elimina valores NaN e Infinitos reemplazándolos por 0 (silencio)
    static void applyNaNKiller(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            auto* channelData = buffer.getWritePointer(channel);
            for (int sample = startSample; sample < startSample + numSamples; ++sample) {
                if (std::isnan(channelData[sample]) || std::isinf(channelData[sample])) {
                    channelData[sample] = 0.0f;
                }
            }
        }
    }

    // Hard Clipper simple para limitar la señal al rango máximo permitido
    static void applyHardClipper(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, float threshold = 1.0f) {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            auto* channelData = buffer.getWritePointer(channel);
            for (int sample = startSample; sample < startSample + numSamples; ++sample) {
                if (channelData[sample] > threshold) {
                    channelData[sample] = threshold;
                } else if (channelData[sample] < -threshold) {
                    channelData[sample] = -threshold;
                }
            }
        }
    }
};