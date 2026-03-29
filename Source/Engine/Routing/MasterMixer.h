#pragma once
#include <JuceHeader.h>
#include "../../Tracks/Track.h"

class MasterMixer {
public:
    static void processTrackVolumeAndRouting(Track* track,
        int numSamples,
        int hardwareOutChannels,
        int parentIndex,
        const juce::Array<Track*>& allTracks,
        const juce::AudioSourceChannelInfo& bufferToFill) noexcept
    {
        if (track->audioBuffer.getNumChannels() > 0) {
            track->currentPeakLevelL = track->audioBuffer.getMagnitude(0, 0, numSamples);
            if (track->audioBuffer.getNumChannels() > 1)
                track->currentPeakLevelR = track->audioBuffer.getMagnitude(1, 0, numSamples);
            else
                track->currentPeakLevelR = track->currentPeakLevelL;
        }

        float v = track->getVolume();
        float pan = track->getBalance();

        if (hardwareOutChannels >= 2 && track->audioBuffer.getNumChannels() >= 2) {
            track->audioBuffer.applyGain(0, 0, numSamples, v * juce::jmin(1.0f, 1.0f - pan));
            track->audioBuffer.applyGain(1, 0, numSamples, v * juce::jmin(1.0f, 1.0f + pan));
        } else if (track->audioBuffer.getNumChannels() == 1) {
            track->audioBuffer.applyGain(0, 0, numSamples, v);
        }

        int channelsToMix = juce::jmin(hardwareOutChannels, track->audioBuffer.getNumChannels());

        if (parentIndex != -1 && parentIndex < allTracks.size()) {
            for (int c = 0; c < channelsToMix; ++c) {
                if(c < allTracks[parentIndex]->audioBuffer.getNumChannels()) {
                    allTracks[parentIndex]->audioBuffer.addFrom(c, 0, track->audioBuffer, c, 0, numSamples);
                }
            }
        }
        else {
            for (int c = 0; c < channelsToMix; ++c)
                bufferToFill.buffer->addFrom(c, bufferToFill.startSample, track->audioBuffer, c, 0, numSamples);
        }
    }
};
