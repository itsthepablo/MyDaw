#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <atomic>
#include <algorithm>

// ==============================================================================
// SimpleMidSide — Analizador especializado por PICOS (Peak) y Amplitud Lineal.
// Procesa el buffer estéreo para extraer picos M, S y correlación de fase.
// Optimizado para visualización tipo vectorscopio/waveform en el tiempo.
// ==============================================================================
class SimpleMidSide
{
public:
    SimpleMidSide() {
        phaseBuffer.resize(2048, 0.0f);
        lSquaredBuffer.resize(2048, 0.0f);
        rSquaredBuffer.resize(2048, 0.0f);
    }

    void prepare(double sampleRate) {
        reset();
    }

    void reset() {
        mPeak.store(0.0f);
        sPeak.store(0.0f);
        phaseCorr.store(1.0f);
        writeIndex = 0;
        std::fill(phaseBuffer.begin(), phaseBuffer.end(), 0.0f);
        std::fill(lSquaredBuffer.begin(), lSquaredBuffer.end(), 0.0f);
        std::fill(rSquaredBuffer.begin(), rSquaredBuffer.end(), 0.0f);
        runningSumLR = 0; runningSumLL = 0; runningSumRR = 0;
    }

    void process(const juce::AudioBuffer<float>& buffer) {
        int numSamples = buffer.getNumSamples();
        int numChannels = buffer.getNumChannels();
        if (numChannels < 2) return;

        const float* lData = buffer.getReadPointer(0);
        const float* rData = buffer.getReadPointer(1);

        float maxM = 0.0f;
        float maxS = 0.0f;

        for (int i = 0; i < numSamples; ++i) {
            float l = lData[i];
            float r = rData[i];

            // 1. Conversión M/S Peak (Amplitud Real)
            float m = std::abs(l + r) * 0.5f;
            float s = std::abs(l - r) * 0.5f;

            if (m > maxM) maxM = m;
            if (s > maxS) maxS = s;

            // 2. Correlación de Fase (Ventana deslizante)
            updatePhaseCorrelation(l, r);
        }

        // 3. Suavizado de Picos (Crecimiento instantáneo, decaimiento lento)
        float currentM = mPeak.load();
        float currentS = sPeak.load();

        if (maxM > currentM) mPeak.store(maxM);
        else mPeak.store(currentM * 0.95f); // Release rápido para reactividad

        if (maxS > currentS) sPeak.store(maxS);
        else sPeak.store(currentS * 0.95f);
    }

    float getMid() const { return mPeak.load(); }
    float getSide() const { return sPeak.load(); }
    float getPhaseCorrelation() const { return phaseCorr.load(); }

private:
    void updatePhaseCorrelation(float l, float r) {
        float lr = l * r;
        float ll = l * l;
        float rr = r * r;

        runningSumLR += lr - phaseBuffer[writeIndex];
        runningSumLL += ll - lSquaredBuffer[writeIndex];
        runningSumRR += rr - rSquaredBuffer[writeIndex];

        phaseBuffer[writeIndex] = lr;
        lSquaredBuffer[writeIndex] = ll;
        rSquaredBuffer[writeIndex] = rr;

        writeIndex = (writeIndex + 1) % phaseBuffer.size();

        float den = std::sqrt((float)(runningSumLL * runningSumRR));
        if (den > 1e-9f) {
            float c = (float)(runningSumLR / den);
            phaseCorr.store(phaseCorr.load() * 0.95f + c * 0.05f);
        }
    }

    std::atomic<float> mPeak{ 0.0f }, sPeak{ 0.0f }, phaseCorr{ 1.0f };
    std::vector<float> phaseBuffer, lSquaredBuffer, rSquaredBuffer;
    double runningSumLR{ 0 }, runningSumLL{ 0 }, runningSumRR{ 0 };
    size_t writeIndex{ 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleMidSide)
};
