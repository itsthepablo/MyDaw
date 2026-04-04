#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <atomic>

// ==============================================================================
// SimpleBalance — Analizador de balance estéreo perceptual.
// Calcula la diferencia de energía (dB) entre L y R usando Weighting K.
// ==============================================================================
class SimpleBalance
{
public:
    SimpleBalance() {}

    void prepare(double sampleRate, int samplesPerBlock)
    {
        fs = sampleRate;
        currentBalance.store(0.0f);

        // --- 1. FILTROS K-WEIGHTING (Idénticos a SimpleLoudness para consistencia perceptual) ---
        // Stage 1: High Shelf (+4dB @ 1.5kHz)
        double f0 = 1500.0, G = 4.0, Q = 1.0 / std::sqrt(2.0);
        double K = std::tan(juce::MathConstants<double>::pi * f0 / fs);
        double V = std::pow(10.0, G / 20.0);
        double norm1 = 1.0 + K / Q + K * K;
        auto c1 = juce::IIRCoefficients(
            (V + std::sqrt(V) * K / Q + K * K) / norm1,
            2.0 * (K * K - V) / norm1,
            (V - std::sqrt(V) * K / Q + K * K) / norm1,
            1.0,
            2.0 * (K * K - 1.0) / norm1,
            (1.0 - K / Q + K * K) / norm1
        );

        // Stage 2: High Pass (Filtro RLB @ 38Hz)
        f0 = 38.0; Q = 0.5;
        K = std::tan(juce::MathConstants<double>::pi * f0 / fs);
        double norm2 = 1.0 + K / Q + K * K;
        auto c2 = juce::IIRCoefficients(
            1.0 / norm2, -2.0 / norm2, 1.0 / norm2,
            1.0,
            2.0 * (K * K - 1.0) / norm2,
            (1.0 - K / Q + K * K) / norm2
        );

        for (int i = 0; i < 2; ++i) {
            kFilterShelf[i].setCoefficients(c1); 
            kFilterHP[i].setCoefficients(c2);
            kFilterShelf[i].reset(); 
            kFilterHP[i].reset();
        }

        // --- 2. BUFFERS DE SUAVIZADO (400ms para Reactividad Estándar) ---
        smoothWindowSize = (int)(fs * 0.4);
        energyBufferL.assign(smoothWindowSize, 0.0);
        energyBufferR.assign(smoothWindowSize, 0.0);
        bufferIndex = 0;
        runningSumL = 0.0;
        runningSumR = 0.0;

        processBuffer.setSize(2, samplesPerBlock);
        reset();
    }

    void reset()
    {
        for (int i = 0; i < 2; ++i) { kFilterShelf[i].reset(); kFilterHP[i].reset(); }
        std::fill(energyBufferL.begin(), energyBufferL.end(), 0.0);
        std::fill(energyBufferR.begin(), energyBufferR.end(), 0.0);
        bufferIndex = 0; 
        runningSumL = 0.0; 
        runningSumR = 0.0;
        currentBalance.store(0.0f);
    }

    void process(const juce::AudioBuffer<float>& buffer)
    {
        int numSamples = buffer.getNumSamples();
        if (numSamples <= 0) return;

        if (processBuffer.getNumSamples() < numSamples) 
            processBuffer.setSize(2, numSamples, false, true, true);

        // 1. Filtrado Perceptual
        for (int ch = 0; ch < juce::jmin(2, buffer.getNumChannels()); ++ch) {
            float* dest = processBuffer.getWritePointer(ch);
            const float* src = buffer.getReadPointer(ch);
            juce::FloatVectorOperations::copy(dest, src, numSamples);
            kFilterShelf[ch].processSamples(dest, numSamples); 
            kFilterHP[ch].processSamples(dest, numSamples);
        }

        const float* L = processBuffer.getReadPointer(0); 
        const float* R = (processBuffer.getNumChannels() > 1) ? processBuffer.getReadPointer(1) : L;

        // 2. Cálculo de Energía y Diferencia
        for (int i = 0; i < numSamples; ++i)
        {
            double pL = (double)(L[i] * L[i]);
            double pR = (double)(R[i] * R[i]);

            runningSumL -= energyBufferL[bufferIndex];
            energyBufferL[bufferIndex] = pL;
            runningSumL += pL;

            runningSumR -= energyBufferR[bufferIndex];
            energyBufferR[bufferIndex] = pR;
            runningSumR += pR;

            bufferIndex = (bufferIndex + 1) % smoothWindowSize;
        }

        double meanL = runningSumL / (double)smoothWindowSize;
        double meanR = runningSumR / (double)smoothWindowSize;

        // 3. Gate de Silencio (-70dB): Si ambos son muy bajos, forzar centro.
        const double epsilon = 1.0e-7; // -70dB en energía aprox
        if (meanL < epsilon && meanR < epsilon) {
            currentBalance.store(0.0f);
            return;
        }

        // 4. Resultado en dB
        float balanceDB = (float)(10.0 * std::log10((meanL + 1e-12) / (meanR + 1e-12)));
        currentBalance.store(juce::jlimit(-24.0f, 24.0f, balanceDB));
    }

    float getBalance() const { return currentBalance.load(); }

private:
    double fs = 44100.0;
    juce::IIRFilter kFilterShelf[2], kFilterHP[2];
    std::vector<double> energyBufferL, energyBufferR;
    juce::AudioBuffer<float> processBuffer;
    int smoothWindowSize = 0, bufferIndex = 0;
    double runningSumL = 0.0, runningSumR = 0.0;
    std::atomic<float> currentBalance { 0.0f };
};
