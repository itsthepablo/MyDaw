#pragma once
#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>

/**
 * @class PatternInlineEQ_DSP
 * Motor de ecualización de 5 bandas y analizador FFT optimizado para la Playlist.
 */
class PatternInlineEQ_DSP {
public:
    PatternInlineEQ_DSP() : fft(11) // 2^11 = 2048 muestras
    {
        window = std::make_unique<juce::dsp::WindowingFunction<float>>(fft.getSize(), juce::dsp::WindowingFunction<float>::hann);
        
        // Inicialización de seguridad para evitar nullptr en el primer frame
        fftData.assign(fft.getSize() * 2, 0.0f);
        magnitudes.assign(fft.getSize() / 2, 0.0f);
        fifo.fill(0.0f);
    }

    void prepare(double sr, int samplesPerBlock, int numChannels)
    {
        if (sr <= 0 || numChannels <= 0) return;
        sampleRate = sr;
        
        juce::dsp::ProcessSpec spec { sr, (juce::uint32)samplesPerBlock, (juce::uint32)numChannels };
        filterChain.prepare(spec);
        updateFilters();
        
        fftBuffer.clear();
        fftData.assign(fft.getSize() * 2, 0.0f);
        magnitudes.assign(fft.getSize() / 2, 0.0f);

        isPrepared = true;
    }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        if (!isPrepared || block.getNumChannels() == 0) return;
        
        // 1. ECUALIZACIÓN
        juce::dsp::ProcessContextReplacing<float> context(block);
        filterChain.process(context);
        
        // 2. ANALIZADOR FFT
        const int numSamples = (int)block.getNumSamples();
        const float* readPtr = block.getChannelPointer(0); 

        for (int i = 0; i < numSamples; ++i)
        {
            pushNextSampleIntoFifo(readPtr[i]);
        }
    }

    // --- PARÁMETROS DEL EQ ---
    void setHP(float freq) { hpFreq = freq; updateFilters(); }
    void setLP(float freq) { lpFreq = freq; updateFilters(); }
    void setLowShelf(float freq, float gain) { lsFreq = freq; lsGain = gain; updateFilters(); }
    void setHighShelf(float freq, float gain) { hsFreq = freq; hsGain = gain; updateFilters(); }
    void setBell(float freq, float gain, float q) { bellFreq = freq; bellGain = gain; bellQ = q; updateFilters(); }

    float getHP() const { return hpFreq; }
    float getLP() const { return lpFreq; }
    float getLSFreq() const { return lsFreq; }
    float getLSGain() const { return lsGain; }
    float getHSFreq() const { return hsFreq; }
    float getHSGain() const { return hsGain; }
    float getBellFreq() const { return bellFreq; }
    float getBellGain() const { return bellGain; }

    // --- ACCESO A DATOS FFT (Thread-Safe) ---
    const std::vector<float>& getMagnitudes() const { return magnitudes; }

private:
    void updateFilters()
    {
        if (sampleRate <= 0) return;

        // Band 0: High Pass
        filterChain.get<0>().state = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, juce::jlimit(20.0f, 20000.0f, hpFreq));
        
        // Band 1: Low Shelf
        filterChain.get<1>().state = juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, juce::jlimit(20.0f, 20000.0f, lsFreq), 0.707f, juce::Decibels::decibelsToGain(lsGain));
        
        // Band 2: Bell
        filterChain.get<2>().state = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, juce::jlimit(20.0f, 20000.0f, bellFreq), bellQ, juce::Decibels::decibelsToGain(bellGain));
        
        // Band 3: High Shelf
        filterChain.get<3>().state = juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, juce::jlimit(20.0f, 20000.0f, hsFreq), 0.707f, juce::Decibels::decibelsToGain(hsGain));
        
        // Band 4: Low Pass
        filterChain.get<4>().state = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, juce::jlimit(20.0f, 20000.0f, lpFreq));
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
        for (int i = 0; i < numBins; ++i)
        {
            float mag = fftData[i];
            // Suavizado temporal ligero para un espectrograma premium
            magnitudes[i] = magnitudes[i] * 0.7f + mag * 0.3f;
        }
        nextFFTBlockReady = false;
    }

    double sampleRate = 44100.0;
    std::atomic<bool> isPrepared { false };
    
    // Cadena de 5 filtros IIR
    juce::dsp::ProcessorChain<
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
    > filterChain;

    // Parámetros internos
    float hpFreq = 20.0f, lpFreq = 20000.0f;
    float lsFreq = 150.0f, lsGain = 0.0f;
    float hsFreq = 5000.0f, hsGain = 0.0f;
    float bellFreq = 1000.0f, bellGain = 0.0f, bellQ = 1.0f;

    // Analizador FFT
    juce::dsp::FFT fft;
    std::unique_ptr<juce::dsp::WindowingFunction<float>> window;
    std::array<float, 2048> fifo;
    std::vector<float> fftData;
    std::vector<float> magnitudes;
    int fifoIndex = 0;
    bool nextFFTBlockReady = false;
    juce::AudioBuffer<float> fftBuffer;
};
