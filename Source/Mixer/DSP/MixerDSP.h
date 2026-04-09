#pragma once
#include <JuceHeader.h>
#include "../../Data/Track.h"

/**
 * MixerDSP: Motor de procesamiento de audio para los canales del mixer.
 * Encapsula la lógica de aplicación de ganancia, paneo y medición.
 */
class MixerDSP {
public:
    static void applyGainAndPan(Track* track, int numSamples, int hardwareOutChannels) noexcept
    {
        // El acceso a mixerData es posible porque MixerDSP es 'friend' de Track
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
                // DUAL PANNING - Linear Balance (0dB Unity) + Smoothed
                const float pL = track->mixerData.panL.load(std::memory_order_relaxed);
                const float pR = track->mixerData.panR.load(std::memory_order_relaxed);

                const float posL = (pL + 1.0f) * 0.5f;
                const float posR = (pR + 1.0f) * 0.5f;

                for (int i = 0; i < numSamples; ++i) {
                    const float smoothV = track->mixerData.volumeSmoother.getNextValue();
                    const float inL = lData[i];
                    const float inR = rData[i];

                    lData[i] = (inL * (1.0f - posL) + inR * (1.0f - posR)) * smoothV;
                    rData[i] = (inL * posL          + inR * posR)          * smoothV;
                }
                track->mixerData.panSmoother.skip(numSamples);
            }
            else 
            {
                // STANDARD PANNING - Linear Balance (0dB Unity) + Smoothed
                for (int i = 0; i < numSamples; ++i) {
                    const float v = track->mixerData.volumeSmoother.getNextValue();
                    const float pan = track->mixerData.panSmoother.getNextValue();

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

        // --- VU-METER Update (Post-Fader / Post-Pan) ---
        if (track->audioBuffer.getNumChannels() > 0) {
            track->mixerData.currentPeakLevelL.store(track->audioBuffer.getMagnitude(0, 0, numSamples), std::memory_order_relaxed);
            track->mixerData.currentPeakLevelR.store(track->audioBuffer.getNumChannels() > 1
                ? track->audioBuffer.getMagnitude(1, 0, numSamples)
                : track->mixerData.currentPeakLevelL.load(), std::memory_order_relaxed);
        }
        
        // --- AQUÍ SE AGREGARÁN FUTURAS FUNCIONES (Solo Mid-Side, EQ interna, etc.) ---
    }
};
