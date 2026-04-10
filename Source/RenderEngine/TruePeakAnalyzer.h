#pragma once
#include <JuceHeader.h>

class TruePeakAnalyzer
{
public:
    TruePeakAnalyzer() {}
    ~TruePeakAnalyzer() {}

    void prepareToPlay(double sampleRate, int samplesPerBlock, int numChannels)
    {
        // Inicializar el oversampler. Factor 2 significa 4x (2^2)
        oversampler = std::make_unique<juce::dsp::Oversampling<float>>(
            numChannels,
            2,
            juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple,
            true, // is phase active
            true // is real time
        );
        oversampler->initProcessing(samplesPerBlock);
        currentTruePeak = 0.0f;
    }

    void processTruePeak(juce::AudioBuffer<float>& buffer)
    {
        if (oversampler == nullptr)
            return;

        juce::dsp::AudioBlock<float> block(buffer);

        // Sobremuestreo 4x de la señal original
        juce::dsp::AudioBlock<float> oversampledBlock = oversampler->processSamplesUp(block);

        // Analizar la curva reconstruida (búsqueda de pico absoluto intermuestral)
        for (int channel = 0; channel < (int)oversampledBlock.getNumChannels(); ++channel)
        {
            auto* channelData = oversampledBlock.getChannelPointer(channel);
            for (int i = 0; i < (int)oversampledBlock.getNumSamples(); ++i)
            {
                float absValue = std::abs(channelData[i]);
                if (absValue > currentTruePeak)
                {
                    currentTruePeak = absValue;
                }
            }
        }

        // Submuestreo inverso obligatorio para que no varíen los samples del audio real
        oversampler->processSamplesDown(block);
    }

    float getTruePeakLinear() const
    {
        return currentTruePeak;
    }
    
    void reset()
    {
        currentTruePeak = 0.0f;
        if (oversampler != nullptr)
            oversampler->reset();
    }

private:
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampler;
    float currentTruePeak = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TruePeakAnalyzer)
};
