#pragma once
#include <JuceHeader.h>
#include <vector>

class StereoAnalyzer
{
public:
    StereoAnalyzer() {}

    void prepare(int maximumSamplesPerBlock)
    {
        // Guardamos los últimos 1024 samples para dibujar (efecto de persistencia visual)
        sampleBuffer.setSize(2, 1024);
        sampleBuffer.clear();
        writePtr = 0;
    }

    void reset()
    {
        sampleBuffer.clear();
        writePtr = 0;
    }

    void process(const juce::AudioBuffer<float>& buffer)
    {
        int numSamples = buffer.getNumSamples();
        int bufferSize = sampleBuffer.getNumSamples();

        const float* inputL = buffer.getReadPointer(0);
        const float* inputR = buffer.getReadPointer(juce::jmin(1, buffer.getNumChannels() - 1));

        for (int i = 0; i < numSamples; ++i)
        {
            sampleBuffer.setSample(0, writePtr, inputL[i]);
            sampleBuffer.setSample(1, writePtr, inputR[i]);

            writePtr = (writePtr + 1) % bufferSize;
        }
    }

    const juce::AudioBuffer<float>& getHistoryBuffer() const { return sampleBuffer; }
    int getCurrentWritePtr() const { return writePtr; }

private:
    juce::AudioBuffer<float> sampleBuffer;
    int writePtr = 0;
};