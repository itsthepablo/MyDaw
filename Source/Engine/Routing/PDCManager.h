#pragma once
#include <JuceHeader.h>
#include "RoutingMatrix.h"
#include "../Core/TransportState.h"

class PDCManager {
public:
    static inline std::atomic<int> currentGlobalLatency { 0 };

    // ------ DIAGNÓSTICO DEL AUDIO THREAD (SOLO DEBUGGING EN VIVO) ------
    static inline std::atomic<int> dbgTracks { 0 };
    static inline std::atomic<int> dbgPlaying { 0 };
    static inline std::atomic<int> dbgClips { 0 };
    static inline std::atomic<int> dbgSamplesWritten { 0 };
    static inline std::atomic<int> dbgAddCount { 0 };
    static inline std::atomic<int> dbgMagCheck { 0 };
    // -------------------------------------------------------------------

    static void calculateLatencies(const RoutingMatrix::TopoState* state, TransportState& ts) noexcept {
        if (!state) return;
        int maxLatency = 0;

        for (auto* track : state->activeTracks) {
            int trackLatency = 0;
            for (auto* p : track->plugins) {
                if (p != nullptr && p->isLoaded()) {
                    trackLatency += p->getLatencySamples();
                }
            }
            track->currentLatency = trackLatency;

            if (trackLatency > maxLatency) {
                maxLatency = trackLatency;
            }
        }

        ts.currentGlobalLatency.store(maxLatency, std::memory_order_relaxed);
        currentGlobalLatency.store(maxLatency, std::memory_order_relaxed);

        for (auto* track : state->activeTracks) {
            track->requiredDelay = maxLatency - track->currentLatency;
        }
    }

    static void applyDelay(Track* track, int numSamples) noexcept {
        int delay = track->requiredDelay;
        int bufferSize = track->pdcBuffer.getNumSamples();

        if (bufferSize == 0) return;

        int numChannels = track->audioBuffer.getNumChannels();
        int safeChannels = juce::jmin(numChannels, track->pdcBuffer.getNumChannels());

        int currentWritePtr = track->pdcWritePtr;

        for (int ch = 0; ch < safeChannels; ++ch) {
            float* audioData = track->audioBuffer.getWritePointer(ch);
            float* delayData = track->pdcBuffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i) {
                int writePos = (currentWritePtr + i) % bufferSize;

                float currentSample = audioData[i];
                delayData[writePos] = currentSample;

                if (delay > 0) {
                    int readPtr = (currentWritePtr + i - delay + bufferSize) % bufferSize;
                    audioData[i] = delayData[readPtr];
                }
            }
        }

        track->pdcWritePtr = (currentWritePtr + numSamples) % bufferSize;
    }
};
