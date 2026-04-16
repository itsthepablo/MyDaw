#include "MixerDSP.h"

void MixerDSP::applyGainAndPan(Track* track, int numSamples, int hardwareOutChannels, double /*phaseStart*/, double /*phaseEnd*/) noexcept
{
    if (track->mixerData.isMuted.load(std::memory_order_relaxed)) {
        track->audioBuffer.clear();
        return;
    }

    if (hardwareOutChannels >= 2 && track->audioBuffer.getNumChannels() >= 2) 
    {
        float* lData = track->audioBuffer.getWritePointer(0);
        float* rData = track->audioBuffer.getWritePointer(1);

        float maxM = 0.0f;
        float maxS = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            // --- MODULACIÓN ADITIVA UNIVERSAL (PROFESSIONAL) ---
            // Estos valores vienen del NativeModulationManager mapeado via LEARN
            const float modV = track->mixerData.modVolumeSmoother.getNextValue();
            const float modP = track->mixerData.modPanSmoother.getNextValue();

            const float inL = lData[i];
            const float inR = rData[i];

            // 1. Matriz M/S
            float mid = (inL + inR) * 0.5f;
            float side = (inL - inR) * 0.5f;
            mid *= track->mixerData.midVolumeSmoother.getNextValue();
            side *= track->mixerData.sideVolumeSmoother.getNextValue();
            
            maxM = std::max(maxM, std::abs(mid));
            maxS = std::max(maxS, std::abs(side));

            float processedL = mid + side;
            float processedR = mid - side;

            // 2. Volumen (Fader + Mod)
            const float baseV = track->mixerData.volumeSmoother.getNextValue();
            const float v = juce::jlimit(0.0f, 2.0f, baseV + modV);

            // 3. Paneo (Knob + Mod)
            const float baseP = track->mixerData.panSmoother.getNextValue();
            const float pan = juce::jlimit(-1.0f, 1.0f, baseP + modP);
            
            // Ley Lineal Estándar (Lo que prefiere el usuario)
            const float gainL = juce::jlimit(0.0f, 1.0f, 1.0f - pan);
            const float gainR = juce::jlimit(0.0f, 1.0f, 1.0f + pan);
            
            lData[i] = processedL * gainL * v;
            rData[i] = processedR * gainR * v;

            // --- CAPTURA CIRCULAR PARA VECTORSCOPIO (ESTILO TP INSPECTOR) ---
            if (track->midSideBuffer.getNumChannels() >= 2) {
                int wPos = track->mixerData.scopeWritePos.load(std::memory_order_relaxed);
                track->midSideBuffer.setSample(0, wPos, lData[i]);
                track->midSideBuffer.setSample(1, wPos, rData[i]);
                track->mixerData.scopeWritePos.store((wPos + 1) % track->midSideBuffer.getNumSamples(), std::memory_order_relaxed);
            }
        }

        track->mixerData.currentMidPeak.store(maxM, std::memory_order_relaxed);
        track->mixerData.currentSidePeak.store(maxS, std::memory_order_relaxed);
    } 
    else if (track->audioBuffer.getNumChannels() == 1) 
    {
        float* data = track->audioBuffer.getWritePointer(0);
        for (int i = 0; i < numSamples; ++i)
        {
            const float v = juce::jlimit(0.0f, 2.0f, track->mixerData.volumeSmoother.getNextValue() + track->mixerData.modVolumeSmoother.getNextValue());
            data[i] *= v;
        }
        track->mixerData.panSmoother.skip(numSamples);
        track->mixerData.modPanSmoother.skip(numSamples);
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
