#pragma once
#include <JuceHeader.h>
#include "../../Tracks/Track.h"

class MasterMixer {
public:
    // ============================================================
    // Aplica el volumen y el balance (pan) al audioBuffer del track.
    // Llamar desde el audio thread DESPUES de TrackProcessor + PDC,
    // mientras el track aun es "privado" (ningun otro thread lo toca).
    // ============================================================
    static void applyGainAndPan(Track* track, int numSamples, int hardwareOutChannels) noexcept
    {
        // Actualizar vumétros (lectura del buffer antes del gain para reflejo pre-fader)
        if (track->audioBuffer.getNumChannels() > 0) {
            track->currentPeakLevelL = track->audioBuffer.getMagnitude(0, 0, numSamples);
            track->currentPeakLevelR = track->audioBuffer.getNumChannels() > 1
                ? track->audioBuffer.getMagnitude(1, 0, numSamples)
                : track->currentPeakLevelL;
        }

        float v   = track->getVolume();
        float pan = track->getBalance();

        if (hardwareOutChannels >= 2 && track->audioBuffer.getNumChannels() >= 2) {
            track->audioBuffer.applyGain(0, 0, numSamples, v * juce::jmin(1.0f, 1.0f - pan));
            track->audioBuffer.applyGain(1, 0, numSamples, v * juce::jmin(1.0f, 1.0f + pan));
        } else if (track->audioBuffer.getNumChannels() == 1) {
            track->audioBuffer.applyGain(0, 0, numSamples, v);
        }
    }

    // ============================================================
    // Enruta el audio del track al output maestro (bufferToFill).
    // Llamar exclusivamente desde el audio thread principal (Fase 2),
    // NUNCA desde workers paralelos — bufferToFill es compartido.
    // ============================================================
    static void routeToMaster(Track* track, int numSamples, int hardwareOutChannels,
                               const juce::AudioSourceChannelInfo& bufferToFill) noexcept
    {
        int channelsToMix = juce::jmin(hardwareOutChannels, track->audioBuffer.getNumChannels());
        for (int c = 0; c < channelsToMix; ++c)
            bufferToFill.buffer->addFrom(c, bufferToFill.startSample,
                                         track->audioBuffer, c, 0, numSamples);
    }

    // ============================================================
    // API LEGACY — mantiene compatibilidad con codigo existente.
    // Equivalente a applyGainAndPan + routeToParent/routeToMaster
    // en una sola llamada, para uso secuencial (no paralelo).
    // ============================================================
    static void processTrackVolumeAndRouting(Track* track,
        int numSamples,
        int hardwareOutChannels,
        int parentIndex,
        const juce::Array<Track*>& allTracks,
        const juce::AudioSourceChannelInfo& bufferToFill) noexcept
    {
        applyGainAndPan(track, numSamples, hardwareOutChannels);

        int channelsToMix = juce::jmin(hardwareOutChannels, track->audioBuffer.getNumChannels());

        if (parentIndex != -1 && parentIndex < allTracks.size()) {
            for (int c = 0; c < channelsToMix; ++c) {
                if (c < allTracks[parentIndex]->audioBuffer.getNumChannels())
                    allTracks[parentIndex]->audioBuffer.addFrom(c, 0, track->audioBuffer, c, 0, numSamples);
            }
        } else {
            routeToMaster(track, numSamples, hardwareOutChannels, bufferToFill);
        }
    }
};
