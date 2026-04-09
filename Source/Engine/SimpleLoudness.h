#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <atomic>

/**
 * SimpleLoudness — Motor de medición de LUFS basado en BS.1770-4.
 * Ubicado en Engine por ser una utilidad compartida de bajo nivel.
 */
class SimpleLoudness
{
public:
    SimpleLoudness();

    void prepare(double sampleRate, int samplesPerBlock);
    void reset();
    void process(const juce::AudioBuffer<float>& buffer);

    float getMomentary() const { return lufsMomentary.load(); }
    float getShortTerm() const { return lufsShort.load(); }
    float getIntegrated() const { return lufsIntegrated.load(); }
    float getTruePeak() const { return lufsTruePeak.load(); }
    float getRange() const { return lufsRange.load(); }

    void calculateIntegratedAndLRA();

private:
    float energyToLufs(double energy);

    double fs = 44100.0;
    juce::IIRFilter kFilterShelf[2], kFilterHP[2];
    std::vector<double> energyBuffer;
    juce::AudioBuffer<float> processBuffer;
    int shortTermSize = 0, bufferIndex = 0;
    double runningSumShort = 0.0;
    std::vector<double> integratedEnergyBlocks;
    std::vector<float> shortTermLufsHistory;
    int blockSize = 0, currentBlockSamples = 0;
    double currentBlockAccumulator = 0.0;
    
    std::atomic<float> lufsMomentary { -70.0f };
    std::atomic<float> lufsShort { -70.0f };
    std::atomic<float> lufsIntegrated { -70.0f };
    std::atomic<float> lufsTruePeak { 0.0f };
    std::atomic<float> lufsRange { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleLoudness)
};