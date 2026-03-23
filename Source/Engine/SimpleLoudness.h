#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <algorithm>

class SimpleLoudness
{
public:
    SimpleLoudness() {}

    void prepare(double sampleRate, int samplesPerBlock)
    {
        fs = sampleRate;

        // --- 1. FILTROS BS.1770-4 (Coeficientes Biquad Manuales) ---
        // Stage 1: High Shelf
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

        // Stage 2: High Pass
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
            kFilterShelf[i].setCoefficients(c1); kFilterHP[i].setCoefficients(c2);
            kFilterShelf[i].reset(); kFilterHP[i].reset();
        }

        // --- 2. BUFFERS ---
        shortTermSize = (int)(fs * 3.0);
        energyBuffer.resize(shortTermSize, 0.0);
        bufferIndex = 0;
        runningSumShort = 0.0;

        blockSize = (int)(fs * 0.1);
        currentBlockAccumulator = 0.0;
        currentBlockSamples = 0;

        integratedEnergyBlocks.clear();
        integratedEnergyBlocks.reserve(6000);
        shortTermLufsHistory.clear();
        shortTermLufsHistory.reserve(6000);

        reset();
    }

    void reset()
    {
        for (int i = 0; i < 2; ++i) { kFilterShelf[i].reset(); kFilterHP[i].reset(); }
        std::fill(energyBuffer.begin(), energyBuffer.end(), 0.0);
        bufferIndex = 0; runningSumShort = 0.0;

        integratedEnergyBlocks.clear();
        shortTermLufsHistory.clear();

        currentBlockAccumulator = 0.0; currentBlockSamples = 0;

        lufsShort = -70.0f; lufsIntegrated = -70.0f; lufsTruePeak = 0.0f; lufsRange = 0.0f;
    }

    void process(const juce::AudioBuffer<float>& buffer)
    {
        int numSamples = buffer.getNumSamples();
        if (numSamples == 0) return;

        // TRUE PEAK
        float maxP = 0.0f;
        maxP = std::max(maxP, buffer.getMagnitude(0, 0, numSamples));
        if (buffer.getNumChannels() > 1) maxP = std::max(maxP, buffer.getMagnitude(1, 0, numSamples));
        if (maxP > lufsTruePeak) lufsTruePeak = maxP; else lufsTruePeak *= 0.999f;

        // FILTRADO
        juce::AudioBuffer<float> kBuf; kBuf.makeCopyOf(buffer);
        for (int ch = 0; ch < 2; ++ch) {
            float* d = kBuf.getWritePointer(ch);
            kFilterShelf[ch].processSamples(d, numSamples); kFilterHP[ch].processSamples(d, numSamples);
        }

        const float* L = kBuf.getReadPointer(0); const float* R = kBuf.getReadPointer(1);

        for (int i = 0; i < numSamples; ++i)
        {
            double power = (double)(L[i] * L[i]) + (double)(R[i] * R[i]);
            if (power < 1.0e-20) power = 1.0e-20;

            double old = energyBuffer[bufferIndex];
            runningSumShort -= old;
            energyBuffer[bufferIndex] = power;
            runningSumShort += power;
            bufferIndex = (bufferIndex + 1) % shortTermSize;

            currentBlockAccumulator += power;
            currentBlockSamples++;

            if (currentBlockSamples >= blockSize) {
                double blockMean = currentBlockAccumulator / (double)currentBlockSamples;
                integratedEnergyBlocks.push_back(blockMean);

                double stMean = runningSumShort / (double)shortTermSize;
                shortTermLufsHistory.push_back(energyToLufs(stMean));

                currentBlockAccumulator = 0.0; currentBlockSamples = 0;
                calculateIntegratedAndLRA();
            }
        }

        double stMean = runningSumShort / (double)shortTermSize;
        lufsShort = energyToLufs(stMean);
    }

    float getShortTerm() { return lufsShort; }
    float getIntegrated() { return lufsIntegrated; }
    float getTruePeak() { return lufsTruePeak; }
    float getRange() { return lufsRange; }

private:
    double fs = 44100.0;
    juce::IIRFilter kFilterShelf[2], kFilterHP[2];
    std::vector<double> energyBuffer;
    int shortTermSize = 0, bufferIndex = 0;
    double runningSumShort = 0.0;
    std::vector<double> integratedEnergyBlocks;
    std::vector<float> shortTermLufsHistory;
    int blockSize = 0, currentBlockSamples = 0;
    double currentBlockAccumulator = 0.0;
    float lufsShort = -70.0f, lufsIntegrated = -70.0f, lufsTruePeak = 0.0f, lufsRange = 0.0f;

    float energyToLufs(double energy) {
        if (energy <= 1.0e-20) return -70.0f;
        return (float)(-0.691 + 10.0 * std::log10(energy));
    }

    void calculateIntegratedAndLRA() {
        if (integratedEnergyBlocks.size() < 4) return;
        std::vector<double> windows;
        size_t n = integratedEnergyBlocks.size();
        windows.reserve(n);
        for (size_t i = 0; i <= n - 4; ++i) {
            windows.push_back((integratedEnergyBlocks[i] + integratedEnergyBlocks[i + 1] + integratedEnergyBlocks[i + 2] + integratedEnergyBlocks[i + 3]) * 0.25);
        }

        double absThresh = 2.5059e-08;
        double sumValid = 0.0;
        int countValid = 0;
        for (double w : windows) { if (w > absThresh) { sumValid += w; countValid++; } }

        if (countValid == 0) { lufsIntegrated = -70.0f; lufsRange = 0.0f; return; }

        double relLoudness = energyToLufs(sumValid / countValid);
        double relThresh = std::pow(10.0, (relLoudness - 10.0 + 0.691) / 10.0);

        double finalSum = 0.0;
        int finalCount = 0;
        for (double w : windows) { if (w > relThresh) { finalSum += w; finalCount++; } }

        if (finalCount > 0) lufsIntegrated = energyToLufs(finalSum / finalCount);
        else lufsIntegrated = -70.0f;

        if (shortTermLufsHistory.empty()) return;

        float gateLRA = lufsIntegrated - 20.0f;
        float lowerGate = std::max(-70.0f, gateLRA);

        std::vector<float> validLRA;
        validLRA.reserve(shortTermLufsHistory.size());

        for (float val : shortTermLufsHistory) {
            if (val > lowerGate) validLRA.push_back(val);
        }

        if (validLRA.size() < 2) { lufsRange = 0.0f; return; }
        std::sort(validLRA.begin(), validLRA.end());

        size_t idx10 = (size_t)(validLRA.size() * 0.10);
        size_t idx95 = (size_t)(validLRA.size() * 0.95);

        if (idx95 >= validLRA.size()) idx95 = validLRA.size() - 1;
        if (idx10 >= idx95) idx10 = (idx95 > 0 ? idx95 - 1 : 0);

        lufsRange = validLRA[idx95] - validLRA[idx10];
    }
};