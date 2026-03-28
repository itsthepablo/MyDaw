#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"

class PDCManager {
public:
    static void calculateLatencies(TrackContainer& trackContainer) {
        int globalMaxLatency = 0;

        // 1. Encontrar cuál es el VST más lento de todo el proyecto
        for (auto* track : trackContainer.getTracks()) {
            int trackLatency = 0;
            for (auto* p : track->plugins) {
                if (p->isLoaded()) {
                    trackLatency += p->getLatencySamples();
                }
            }
            track->currentLatency = trackLatency;

            if (trackLatency > globalMaxLatency) {
                globalMaxLatency = trackLatency;
            }
        }

        // 2. Calcular cuánto retraso necesita cada pista para igualar a la más lenta
        for (auto* track : trackContainer.getTracks()) {
            track->requiredDelay = globalMaxLatency - track->currentLatency;
        }
    }

    static void applyDelay(Track* track, int numSamples) {
        int delay = track->requiredDelay;
        int bufferSize = track->pdcBuffer.getNumSamples();

        // Seguridad Anti-Crash
        if (bufferSize == 0 || delay >= bufferSize) return;

        int numChannels = track->audioBuffer.getNumChannels();
        int safeChannels = juce::jmin(numChannels, track->pdcBuffer.getNumChannels());

        int currentWritePtr = track->pdcWritePtr;

        // Búfer Circular Perfeccionado (Zero-Allocation)
        for (int ch = 0; ch < safeChannels; ++ch) {
            float* audioData = track->audioBuffer.getWritePointer(ch);
            float* delayData = track->pdcBuffer.getWritePointer(ch);

            for (int i = 0; i < numSamples; ++i) {
                int writePos = (currentWritePtr + i) % bufferSize;

                // 1. SIEMPRE grabamos el audio presente en el historial
                float currentSample = audioData[i];
                delayData[writePos] = currentSample;

                // 2. Si la pista necesita retrasarse, sobrescribimos la salida leyendo el pasado
                // Si delay es 0, dejamos el audioData intacto (0 latencia)
                if (delay > 0) {
                    int readPtr = (currentWritePtr + i - delay + bufferSize) % bufferSize;
                    audioData[i] = delayData[readPtr];
                }
            }
        }

        // Avanzar el cabezal del historial
        track->pdcWritePtr = (currentWritePtr + numSamples) % bufferSize;
    }
};