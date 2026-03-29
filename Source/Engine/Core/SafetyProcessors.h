#pragma once
#include <JuceHeader.h>
#include <cmath>

class SafetyProcessors {
public:
    static void applyNaNKiller(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) noexcept {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            auto* channelData = buffer.getWritePointer(channel) + startSample;
            for (int sample = 0; sample < numSamples; ++sample) {
                if (std::isnan(channelData[sample]) || std::isinf(channelData[sample])) {
                    channelData[sample] = 0.0f;
                }
            }
        }
    }

    static void applyHardClipper(juce::AudioBuffer<float>& buffer, int startSample, int numSamples, float threshold = 1.0f) noexcept {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            auto* channelData = buffer.getWritePointer(channel) + startSample;
            juce::FloatVectorOperations::clip(channelData, channelData, -threshold, threshold, numSamples);
        }
    }
};
