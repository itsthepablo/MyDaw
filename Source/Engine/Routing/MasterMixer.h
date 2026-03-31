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
        // 1. PHASE INVERSION (Corregido: Argumentos de applyGain)
        if (track->isPhaseInverted) {
            track->audioBuffer.applyGain(0, 0, numSamples, -1.0f);
            if (track->audioBuffer.getNumChannels() > 1)
                track->audioBuffer.applyGain(1, 0, numSamples, -1.0f);
        }

        // 2. VOLUME & PAN PREP
        float v = track->getVolume();
        if (track->isMuted) v = 0.0f;

        // --- Paneo y Ganancia ---
        if (hardwareOutChannels >= 2 && track->audioBuffer.getNumChannels() >= 2) 
        {
            if (track->panningModeDual.load(std::memory_order_relaxed)) 
            {
                float pL = track->panL.load(std::memory_order_relaxed);
                float pR = track->panR.load(std::memory_order_relaxed);

                float* lData = track->audioBuffer.getWritePointer(0);
                float* rData = track->audioBuffer.getWritePointer(1);

                for (int i = 0; i < numSamples; ++i) {
                    float inL = lData[i];
                    float inR = rData[i];

                    // Mapeamos de [-1, 1] a [0, 1] (0 = Hard Left, 1 = Hard Right)
                    float posL = (pL + 1.0f) * 0.5f;
                    float posR = (pR + 1.0f) * 0.5f;

                    // El canal Izquierdo de entrada se posiciona según pL
                    // El canal Derecho de entrada se posiciona según pR
                    lData[i] = (inL * (1.0f - posL) + inR * (1.0f - posR)) * v;
                    rData[i] = (inL * posL          + inR * posR)          * v;
                }
            }
            else 
            {
                float pan = track->getBalance();
                track->audioBuffer.applyGain(0, 0, numSamples, v * juce::jmin(1.0f, 1.0f - pan));
                track->audioBuffer.applyGain(1, 0, numSamples, v * juce::jmin(1.0f, 1.0f + pan));
            }
        } 
        else if (track->audioBuffer.getNumChannels() == 1) 
        {
            track->audioBuffer.applyGain(0, 0, numSamples, v);
        }

        // 3. VU-METER Update (Post-Fader / Post-Pan)
        if (track->audioBuffer.getNumChannels() > 0) {
            track->currentPeakLevelL = track->audioBuffer.getMagnitude(0, 0, numSamples);
            track->currentPeakLevelR = track->audioBuffer.getNumChannels() > 1
                ? track->audioBuffer.getMagnitude(1, 0, numSamples)
                : track->currentPeakLevelL;
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
    // Enruta el audio resumido de un track a otro track (ej. al MASTER).
    // ============================================================
    static void routeToTrack(Track* track, Track* destination, int numSamples) noexcept
    {
        int channelsToMix = juce::jmin(destination->audioBuffer.getNumChannels(), track->audioBuffer.getNumChannels());
        for (int c = 0; c < channelsToMix; ++c)
            destination->audioBuffer.addFrom(c, 0, track->audioBuffer, c, 0, numSamples);
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
