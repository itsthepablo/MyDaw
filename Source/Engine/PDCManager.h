#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"

class PDCManager {
public:
    // Exponemos la latencia global para que el medidor visual la lea
    static inline int currentGlobalLatency = 0;

    static void calculateLatencies(TrackContainer& trackContainer) {
        int maxLatency = 0;

        for (auto* track : trackContainer.getTracks()) {
            int trackLatency = 0;
            for (auto* p : track->plugins) {
                if (p->isLoaded()) {
                    trackLatency += p->getLatencySamples();
                }
            }
            track->currentLatency = trackLatency;

            if (trackLatency > maxLatency) {
                maxLatency = trackLatency;
            }
        }

        currentGlobalLatency = maxLatency; // Actualizamos el panel visual

        for (auto* track : trackContainer.getTracks()) {
            track->requiredDelay = maxLatency - track->currentLatency;
        }
    }

    static void applyDelay(Track* track, int numSamples) {
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

                // Siempre grabamos la realidad en el historial
                float currentSample = audioData[i];
                delayData[writePos] = currentSample;

                // Sobrescribimos la salida leyendo el pasado
                if (delay > 0) {
                    int readPtr = (currentWritePtr + i - delay + bufferSize) % bufferSize;
                    audioData[i] = delayData[readPtr];
                }
            }
        }

        track->pdcWritePtr = (currentWritePtr + numSamples) % bufferSize;
    }
};