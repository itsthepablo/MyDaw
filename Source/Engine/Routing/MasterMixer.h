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
        if (track->mixerData.isMuted.load(std::memory_order_relaxed)) {
            track->audioBuffer.clear();
            return;
        }

        if (hardwareOutChannels >= 2 && track->audioBuffer.getNumChannels() >= 2) 
        {
            float* lData = track->audioBuffer.getWritePointer(0);
            float* rData = track->audioBuffer.getWritePointer(1);

            if (track->mixerData.panningModeDual.load(std::memory_order_relaxed)) 
            {
                // DUAL PANNING - Linear Balance (0dB Unity) + Smoothed (Rule #18)
                const float pL = track->mixerData.panL.load(std::memory_order_relaxed);
                const float pR = track->mixerData.panR.load(std::memory_order_relaxed);

                // Mapeamos de [-1, 1] a [0, 1] (0 = Hard Left, 1 = Hard Right)
                const float posL = (pL + 1.0f) * 0.5f;
                const float posR = (pR + 1.0f) * 0.5f;

                for (int i = 0; i < numSamples; ++i) {
                    const float smoothV = track->mixerData.volumeSmoother.getNextValue();
                    const float inL = lData[i];
                    const float inR = rData[i];

                    // Cada entrada (L y R) se posiciona linealmente según sus knobs
                    lData[i] = (inL * (1.0f - posL) + inR * (1.0f - posR)) * smoothV;
                    rData[i] = (inL * posL          + inR * posR)          * smoothV;
                }
                track->mixerData.panSmoother.skip(numSamples);
            }
            else 
            {
                // STANDARD PANNING - Linear Balance (0dB Unity) + Smoothed (Commercial Grade)
                for (int i = 0; i < numSamples; ++i) {
                    const float v = track->mixerData.volumeSmoother.getNextValue();
                    const float pan = track->mixerData.panSmoother.getNextValue();

                    // Ley de Balance Lineal: 0dB en el centro, 0dB en los extremos.
                    const float gainL = juce::jmin(1.0f, 1.0f - pan);
                    const float gainR = juce::jmin(1.0f, 1.0f + pan);
                    
                    lData[i] *= gainL * v;
                    rData[i] *= gainR * v;
                }
            }
        } 
        else if (track->audioBuffer.getNumChannels() == 1) 
        {
            float* data = track->audioBuffer.getWritePointer(0);
            for (int i = 0; i < numSamples; ++i)
                data[i] *= track->mixerData.volumeSmoother.getNextValue();
            
            track->mixerData.panSmoother.skip(numSamples);
        }

        // 3. VU-METER Update (Post-Fader / Post-Pan)
        if (track->audioBuffer.getNumChannels() > 0) {
            track->mixerData.currentPeakLevelL.store(track->audioBuffer.getMagnitude(0, 0, numSamples), std::memory_order_relaxed);
            track->mixerData.currentPeakLevelR.store(track->audioBuffer.getNumChannels() > 1
                ? track->audioBuffer.getMagnitude(1, 0, numSamples)
                : track->mixerData.currentPeakLevelL.load(), std::memory_order_relaxed);
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
