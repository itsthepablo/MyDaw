#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

/*
  ==============================================================================
    SimpleMeter.h
    - RMS: Ajustado a 50ms (Ultra Rápido) para monitoreo de energía instantánea.
    - Peak: Detección instantánea con caída suave.
  ==============================================================================
*/

class SimpleMeter
{
public:
    SimpleMeter() {}

    void prepare(double sampleRate)
    {
        fs = sampleRate;

        // --- CAMBIO DE VELOCIDAD: ULTRA RÁPIDO ---
        // 300ms = VU Standard (Lento)
        // 50ms  = Energy Snappy (Lo que tú pides)
        float rmsTimeMs = 50.0f;

        // Cálculo del coeficiente del filtro (Alpha)
        rmsAlpha = 1.0f - std::exp(-1.0f / ((rmsTimeMs / 1000.0f) * fs));

        // Peak Release (tiempo de caída del pico)
        // Lo dejamos en 800ms para que sea agradable de ver caer
        float peakReleaseMs = 800.0f;
        peakDecay = 1.0f - std::exp(-1.0f / ((peakReleaseMs / 1000.0f) * fs));

        reset();
    }

    void reset()
    {
        currentRmsSum = 0.0f;
        currentPeak = 0.0f;
    }

    void process(const float* channelData, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = channelData[i];
            float absSample = std::abs(sample);
            float squared = sample * sample;

            // --- 1. CÁLCULO RMS (Filtro rápido) ---
            currentRmsSum += (squared - currentRmsSum) * rmsAlpha;

            // --- 2. CÁLCULO PEAK ---
            if (absSample > currentPeak) {
                currentPeak = absSample;
            }
            else {
                currentPeak += (0.0f - currentPeak) * peakDecay;
            }
        }
    }

    float getRMS() const {
        // Raíz cuadrada de la media de energía
        return std::sqrt(std::max(0.0f, currentRmsSum));
    }

    float getPeak() const {
        return currentPeak;
    }

private:
    double fs = 44100.0;
    float rmsAlpha = 0.0f;
    float currentRmsSum = 0.0f;
    float peakDecay = 0.0f;
    float currentPeak = 0.0f;
};