#pragma once
#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>
#include <memory>
#include <array>
#include <cmath>

/**
 * @class ChannelEQ_DSP
 * Motor de ecualización de alto rendimiento (2 bandas estilo Serum) para cada canal.
 * Incluye analizador FFT incorporado para visualización en tiempo real.
 */
class ChannelEQ_DSP {
public:
    enum BandType { HighPass = 0, LowShelf, Bell, HighShelf, LowPass };

    std::atomic<bool> userBypass { false };

    ChannelEQ_DSP() : fft(11) 
    {
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fft.getSize(), juce::dsp::WindowingFunction<float>::hann);
        fftData.assign(fft.getSize() * 2, 0.0f);
        magnitudes.assign(fft.getSize() / 2, 0.0f);
        fifo.fill(0.0f);
        
        // --- INICIALIZACIÓN ROBUSTA (Regla #1) ---
        sampleRate = 44100.0;
        isPrepared.store(false); // Empezar en falso por seguridad
        
        // Asignar coeficientes de identidad distintos para cada banda para evitar que compartan el mismo puntero y se pisen
        filterChain.get<0>().state = juce::dsp::IIR::Coefficients<float>::makePeakFilter(44100.0, 1000.0, 0.707f, 1.0f);
        filterChain.get<1>().state = juce::dsp::IIR::Coefficients<float>::makePeakFilter(44100.0, 1000.0, 0.707f, 1.0f);
        updateFilters();
    }

    void prepare(double sr, int samplesPerBlock, int numChannels)
    {
        if (sr <= 0 || numChannels <= 0) return;
        sampleRate = sr;
        
        juce::dsp::ProcessSpec spec { sr, (juce::uint32)samplesPerBlock, (juce::uint32)numChannels };
        filterChain.prepare(spec);
        updateFilters();
        
        fftData.assign(fft.getSize() * 2, 0.0f);
        magnitudes.assign(fft.getSize() / 2, 0.0f);
        isPrepared.store(true);
    }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        if (!isPrepared.load() || block.getNumChannels() == 0) return;
        if (userBypass.load(std::memory_order_relaxed)) return; // Bypassed por el usuario
        
        
        // 1. Smart Bypass System: Si los parametros no alteran el espectro, pasamos audio original sin tocar fase.
        bool b1IsNeutral = (b1Gain == 0.0f) && (b1Type != HighPass && b1Type != LowPass);
        bool b2IsNeutral = (b2Gain == 0.0f) && (b2Type != HighPass && b2Type != LowPass);
        
        filterChain.setBypassed<0>(b1IsNeutral);
        filterChain.setBypassed<1>(b2IsNeutral);

        // 2. DSP Processing
        juce::dsp::ProcessContextReplacing<float> context(block);
        filterChain.process(context);
        
        // 2. FFT Shadow Recording (Single Channel for efficiency)
        const int numSamples = (int)block.getNumSamples();
        const float* readPtr = block.getChannelPointer(0); 

        for (int i = 0; i < numSamples; ++i)
            pushNextSampleIntoFifo(readPtr[i]);
    }

    // --- BAND 1 PARAMETERS ---
    void setBand1Type(BandType type) { b1Type = type; updateFilters(); }
    void setBand1Freq(float f) { b1Freq = f; updateFilters(); }
    void setBand1Q(float q) { b1Q = q; updateFilters(); }
    void setBand1Gain(float g) { b1Gain = g; updateFilters(); }

    // --- BAND 2 PARAMETERS ---
    void setBand2Type(BandType type) { b2Type = type; updateFilters(); }
    void setBand2Freq(float f) { b2Freq = f; updateFilters(); }
    void setBand2Q(float q) { b2Q = q; updateFilters(); }
    void setBand2Gain(float g) { b2Gain = g; updateFilters(); }

    BandType getBand1Type() const { return b1Type; }
    float getBand1Freq() const { return b1Freq; }
    float getBand1Q() const { return b1Q; }
    float getBand1Gain() const { return b1Gain; }

    BandType getBand2Type() const { return b2Type; }
    float getBand2Freq() const { return b2Freq; }
    float getBand2Q() const { return b2Q; }
    float getBand2Gain() const { return b2Gain; }

    const std::vector<float>& getMagnitudes() const { return magnitudes; }

    /**
     * Calcula la magnitud real de la respuesta en frecuencia de las dos bandas combinadas.
     * Esencial para una visualización de Grado Comercial (profesional).
     */
    /**
     * Calcula la magnitud real de la respuesta en frecuencia de las dos bandas combinadas.
     * Usa el Sample Rate interno para máxima precisión visual.
     */
    float getMagnitudeForFrequency(float frequency) const
    {
        if (sampleRate <= 0) return 1.0f;
        
        auto mag1 = filterChain.get<0>().state->getMagnitudeForFrequency(frequency, sampleRate);
        auto mag2 = filterChain.get<1>().state->getMagnitudeForFrequency(frequency, sampleRate);
        
        return (float)mag1 * (float)mag2;
    }

private:
    void updateFilters()
    {
        if (sampleRate <= 0) return;

        // --- UPDATE BAND 1 ---
        auto& filter1 = filterChain.get<0>();
        if (filter1.state != nullptr) {
            switch (b1Type) {
                case HighPass: *filter1.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, b1Freq, b1Q); break;
                case LowShelf: *filter1.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, b1Freq, b1Q, juce::Decibels::decibelsToGain(b1Gain)); break;
                case Bell:     *filter1.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, b1Freq, b1Q, juce::Decibels::decibelsToGain(b1Gain)); break;
                default:       *filter1.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, b1Freq, b1Q, 1.0f); break;
            }
        }

        // --- UPDATE BAND 2 ---
        auto& filter2 = filterChain.get<1>();
        if (filter2.state != nullptr) {
            switch (b2Type) {
                case LowPass:   *filter2.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, b2Freq, b2Q); break;
                case HighShelf: *filter2.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, b2Freq, b2Q, juce::Decibels::decibelsToGain(b2Gain)); break;
                case Bell:      *filter2.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, b2Freq, b2Q, juce::Decibels::decibelsToGain(b2Gain)); break;
                case HighPass:  *filter2.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, b2Freq, b2Q); break;
                case LowShelf:  *filter2.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, b2Freq, b2Q, juce::Decibels::decibelsToGain(b2Gain)); break;
                default:        *filter2.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, b2Freq, b2Q, 1.0f); break;
            }
        }
    }

    void pushNextSampleIntoFifo(float sample) noexcept
    {
        if (fifoIndex == fft.getSize())
        {
            if (!nextFFTBlockReady)
            {
                std::fill(fftData.begin(), fftData.end(), 0.0f);
                std::copy(fifo.begin(), fifo.end(), fftData.begin());
                nextFFTBlockReady = true;
                performFFT();
            }
            fifoIndex = 0;
        }
        fifo[fifoIndex++] = sample;
    }

    void performFFT()
    {
        window->multiplyWithWindowingTable(fftData.data(), fft.getSize());
        fft.performFrequencyOnlyForwardTransform(fftData.data());

        const int numBins = fft.getSize() / 2;
        for (int i = 0; i < numBins; ++i) {
            float mag = fftData[i];
            magnitudes[i] = magnitudes[i] * 0.7f + mag * 0.3f;
        }
        nextFFTBlockReady = false;
    }

    double sampleRate = 44100.0;
    std::atomic<bool> isPrepared { false };
    
    
    juce::dsp::ProcessorChain<
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
    > filterChain;

    // Default Params (B1: Low-Shelf, B2: High-Shelf)
    BandType b1Type = LowShelf;
    float b1Freq = 150.0f, b1Gain = 0.0f, b1Q = 0.707f;

    BandType b2Type = HighShelf;
    float b2Freq = 5000.0f, b2Gain = 0.0f, b2Q = 0.707f;

    // FFT Data
    juce::dsp::FFT fft;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    std::array<float, 2048> fifo;
    std::vector<float> fftData;
    std::vector<float> magnitudes;
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
};
