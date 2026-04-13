#pragma once
#include <JuceHeader.h>
#include "../../Data/Track.h"

/**
 * MixerDSP: Motor de procesamiento de audio para los canales del mixer.
 * Encapsula la lógica de aplicación de ganancia, paneo y medición.
 */
class MixerDSP {
public:
    static void applyGainAndPan(Track* track, int numSamples, int hardwareOutChannels, double phaseStart, double phaseEnd) noexcept
    {
        if (track->mixerData.isMuted.load(std::memory_order_relaxed)) {
            track->audioBuffer.clear();
            return;
        }

        const double phaseDelta = (phaseEnd - phaseStart) / (double)numSamples;

        if (hardwareOutChannels >= 2 && track->audioBuffer.getNumChannels() >= 2) 
        {
            float* lData = track->audioBuffer.getWritePointer(0);
            float* rData = track->audioBuffer.getWritePointer(1);

            float maxM = 0.0f;
            float maxS = 0.0f;

            for (int i = 0; i < numSamples; ++i) {
                double currentPhase = phaseStart + (phaseDelta * i);
                
                // --- ACUMULACIÓN DE MODULACIÓN BASADA EN TARGET ---
                float vModAccum = 0.0f;
                float pModAccum = 0.0f;

                for (auto* m : track->modulators) {
                    float val = m->getValueAt(currentPhase);
                    const juce::ScopedLock sl(m->targetsLock); 
                    for (const auto& target : m->targets) {
                        if (target.type == ModTarget::Volume) vModAccum += val;
                        else if (target.type == ModTarget::Pan) pModAccum += val;
                    }
                }

                const float inL = lData[i];
                const float inR = rData[i];

                float mid = (inL + inR) * 0.5f;
                float side = (inL - inR) * 0.5f;

                mid *= track->mixerData.midVolumeSmoother.getNextValue();
                side *= track->mixerData.sideVolumeSmoother.getNextValue();

                maxM = std::max(maxM, std::abs(mid));
                maxS = std::max(maxS, std::abs(side));

                lData[i] = mid + side;
                rData[i] = mid - side;

                // Aplicar Master Volume y Pan
                const float baseV = track->mixerData.volumeSmoother.getNextValue();
                const float baseP = track->mixerData.panSmoother.getNextValue();
                
                // El modulador actúa como una automatización de -1.0 a 1.0. Multiplicamos para volumen, sumamos para pan.
                const float v = juce::jlimit(0.0f, 2.0f, baseV * (1.0f + vModAccum));
                const float pan = juce::jlimit(-1.0f, 1.0f, baseP + pModAccum);

                const float gainL = juce::jmin(1.0f, 1.0f - pan);
                const float gainR = juce::jmin(1.0f, 1.0f + pan);
                
                lData[i] *= gainL * v;
                rData[i] *= gainR * v;
            }
            track->mixerData.currentMidPeak.store(maxM, std::memory_order_relaxed);
            track->mixerData.currentSidePeak.store(maxS, std::memory_order_relaxed);
        } 
        else if (track->audioBuffer.getNumChannels() == 1) 
        {
            float* data = track->audioBuffer.getWritePointer(0);
            for (int i = 0; i < numSamples; ++i)
            {
                double currentPhase = phaseStart + (phaseDelta * i);
                float vModAccum = 0.0f;
                for (auto* m : track->modulators) {
                    float val = m->getValueAt(currentPhase);
                    const juce::ScopedLock sl(m->targetsLock);
                    for (const auto& target : m->targets) {
                        if (target.type == ModTarget::Volume) vModAccum += val;
                    }
                }

                const float baseV = track->mixerData.volumeSmoother.getNextValue();
                const float v = juce::jlimit(0.0f, 2.0f, baseV * (1.0f + vModAccum));
                data[i] *= v;
            }
            track->mixerData.panSmoother.skip(numSamples);
        }

        // --- VU-METER Update ---
        if (track->audioBuffer.getNumChannels() > 0) {
            if (track->audioBuffer.getNumChannels() >= 2) {
                float* lData = track->audioBuffer.getWritePointer(0);
                float* rData = track->audioBuffer.getWritePointer(1);

                if (track->mixerData.isMidSolo.load(std::memory_order_relaxed)) {
                    for (int i = 0; i < numSamples; ++i) {
                        float mid = (lData[i] + rData[i]) * 0.5f;
                        lData[i] = rData[i] = mid;
                    }
                } else if (track->mixerData.isSideSolo.load(std::memory_order_relaxed)) {
                    for (int i = 0; i < numSamples; ++i) {
                        float mid = (lData[i] + rData[i]) * 0.5f;
                        lData[i] -= mid;
                        rData[i] -= mid;
                    }
                }
            }

            track->mixerData.currentPeakLevelL.store(track->audioBuffer.getMagnitude(0, 0, numSamples), std::memory_order_relaxed);
            track->mixerData.currentPeakLevelR.store(track->audioBuffer.getNumChannels() > 1
                ? track->audioBuffer.getMagnitude(1, 0, numSamples)
                : track->mixerData.currentPeakLevelL.load(), std::memory_order_relaxed);
        }
    }
};
