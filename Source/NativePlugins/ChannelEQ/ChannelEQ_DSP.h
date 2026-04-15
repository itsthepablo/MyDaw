#pragma once
#include <JuceHeader.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>
#include <memory>
#include "../../Engine/Modulation/NativeVisualSync.h"
#include "../FFTAnalyzerPlugin/SimpleAnalyzer.h"

/**
 * Motor de ecualización de alto rendimiento (2 bandas estilo Serum) para cada canal.
 * Expandido para soportar modulación universal nativa.
 */
class ChannelEQ_DSP {
public:
    enum BandType { HighPass = 0, LowShelf, Bell, HighShelf, LowPass };

    std::atomic<bool> userBypass { false };

    ChannelEQ_DSP() 
    {
        analyzer.setup(12, 44100.0);
        analyzer.setMasterMode(false); 
        analyzer.setCalibration(16.0f);
        
        sampleRate = 44100.0;
        isPrepared.store(false);

        // Inicializar smoothers de modulación
        modB1Freq.reset(44100.0, 0.02);
        modB1Gain.reset(44100.0, 0.02);
        modB1Q.reset(44100.0, 0.02);
        modB2Freq.reset(44100.0, 0.02);
        modB2Gain.reset(44100.0, 0.02);
        modB2Q.reset(44100.0, 0.02);
        
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

        modB1Freq.reset(sr, 0.02);
        modB1Gain.reset(sr, 0.02);
        modB1Q.reset(sr, 0.02);
        modB2Freq.reset(sr, 0.02);
        modB2Gain.reset(sr, 0.02);
        modB2Q.reset(sr, 0.02);

        updateFilters();
        
        analyzer.setup(12, sr);
        isPrepared.store(true);
    }

    void process(juce::dsp::AudioBlock<float>& block)
    {
        if (!isPrepared.load() || block.getNumChannels() == 0) return;
        if (userBypass.load(std::memory_order_relaxed)) return;
        
        int numSamples = (int)block.getNumSamples();

        // --- ACTUALIZAR FILTROS AL INICIO DEL BLOQUE ---
        updateFilters();

        // 1. Smart Bypass
        bool b1IsNeutral = (b1Gain == 0.0f) && (b1Type != HighPass && b1Type != LowPass);
        bool b2IsNeutral = (b2Gain == 0.0f) && (b2Type != HighPass && b2Type != LowPass);
        
        filterChain.setBypassed<0>(b1IsNeutral);
        filterChain.setBypassed<1>(b2IsNeutral);

        // 2. EQ Processing
        juce::dsp::ProcessContextReplacing<float> context(block);
        filterChain.process(context);
        
        // 3. FFT Analysis
        analyzer.pushBuffer(block.getChannelPointer(0), numSamples);

        // --- AVANZAR SMOOTHERS (CRÍTICO PARA AUTOMATIZACIÓN) ---
        modB1Freq.skip(numSamples);
        modB1Gain.skip(numSamples);
        modB1Q.skip(numSamples);
        modB2Freq.skip(numSamples);
        modB2Gain.skip(numSamples);
        modB2Q.skip(numSamples);
    }

    void processAnalyzer() { analyzer.process(); }

    // --- PARAMS ---
    void setBand1Type(BandType type) { b1Type = type; updateFilters(); }
    void setBand1Freq(float f) { b1Freq = f; updateFilters(); }
    void setBand1Q(float q) { b1Q = q; updateFilters(); }
    void setBand1Gain(float g) { b1Gain = g; updateFilters(); }

    void setBand2Type(BandType type) { b2Type = type; updateFilters(); }
    void setBand2Freq(float f) { b2Freq = f; updateFilters(); }
    void setBand2Q(float q) { b2Q = q; updateFilters(); }
    void setBand2Gain(float g) { b2Gain = g; updateFilters(); }

    // --- INTERFAZ DE MODULACIÓN NATIVA (Caja Negra) ---
    void applyModB1Freq(float unipolar) {
        float f = 20.0f * std::pow(20000.0f / 20.0f, unipolar);
        modB1Freq.setTargetValue(f); visSync.b1Freq.store(f); visSync.hasB1Freq.store(true);
    }
    void applyModB1Gain(float unipolar) {
        float g = juce::jmap(unipolar, 0.0f, 1.0f, -24.0f, 24.0f);
        modB1Gain.setTargetValue(g); visSync.b1Gain.store(g); visSync.hasB1Gain.store(true);
    }
    void applyModB1Q(float unipolar) {
        float q = juce::jmap(unipolar, 0.0f, 1.0f, 0.1f, 10.0f);
        modB1Q.setTargetValue(q); visSync.b1Q.store(q); visSync.hasB1Q.store(true);
    }
    void applyModB2Freq(float unipolar) {
        float f = 20.0f * std::pow(20000.0f / 20.0f, unipolar);
        modB2Freq.setTargetValue(f); visSync.b2Freq.store(f); visSync.hasB2Freq.store(true);
    }
    void applyModB2Gain(float unipolar) {
        float g = juce::jmap(unipolar, 0.0f, 1.0f, -24.0f, 24.0f);
        modB2Gain.setTargetValue(g); visSync.b2Gain.store(g); visSync.hasB2Gain.store(true);
    }
    void applyModB2Q(float unipolar) {
        float q = juce::jmap(unipolar, 0.0f, 1.0f, 0.1f, 10.0f);
        modB2Q.setTargetValue(q); visSync.b2Q.store(q); visSync.hasB2Q.store(true);
    }

    void resetModulations() {
        if (!visSync.hasB1Freq.load()) modB1Freq.setTargetValue(b1Freq);
        if (!visSync.hasB1Gain.load()) modB1Gain.setTargetValue(b1Gain);
        if (!visSync.hasB1Q.load())    modB1Q.setTargetValue(b1Q);
        if (!visSync.hasB2Freq.load()) modB2Freq.setTargetValue(b2Freq);
        if (!visSync.hasB2Gain.load()) modB2Gain.setTargetValue(b2Gain);
        if (!visSync.hasB2Q.load())    modB2Q.setTargetValue(b2Q);
        
        visSync.hasB1Freq.store(false); visSync.hasB1Gain.store(false); visSync.hasB1Q.store(false);
        visSync.hasB2Freq.store(false); visSync.hasB2Gain.store(false); visSync.hasB2Q.store(false);
    }

    // --- PARAMS (Getters) ---
    BandType getBand1Type() const { return b1Type; }
    float getBand1Freq() const { return b1Freq; }
    float getBand1Q() const { return b1Q; }
    float getBand1Gain() const { return b1Gain; }
    BandType getBand2Type() const { return b2Type; }
    float getBand2Freq() const { return b2Freq; }
    float getBand2Q() const { return b2Q; }
    float getBand2Gain() const { return b2Gain; }

    const std::vector<float>& getMagnitudes() { return analyzer.getSpectrumData(); }

    float getMagnitudeForFrequency(float frequency) const
    {
        if (sampleRate <= 0) return 1.0f;
        auto mag1 = filterChain.get<0>().state->getMagnitudeForFrequency(frequency, sampleRate);
        auto mag2 = filterChain.get<1>().state->getMagnitudeForFrequency(frequency, sampleRate);
        return (float)mag1 * (float)mag2;
    }

    // --- PARÁMETROS BASE (Públicos para acceso del Manager) ---
    BandType b1Type = LowShelf;
    float b1Freq = 150.0f, b1Gain = 0.0f, b1Q = 0.707f;

    BandType b2Type = HighShelf;
    float b2Freq = 5000.0f, b2Gain = 0.0f, b2Q = 0.707f;

    // --- SMOOTHING TARGETS ---
    juce::LinearSmoothedValue<float> modB1Freq;
    juce::LinearSmoothedValue<float> modB1Gain;
    juce::LinearSmoothedValue<float> modB1Q;
    juce::LinearSmoothedValue<float> modB2Freq;
    juce::LinearSmoothedValue<float> modB2Gain;
    juce::LinearSmoothedValue<float> modB2Q;

    // --- MODULACIÓN VISUAL (Aislada) ---
    NativeVisualSync visSync;

private:
    void updateFilters()
    {
        if (sampleRate <= 0) return;

        // Lectura directa de valores absolutos (como estaba antes)
        float f1 = juce::jlimit(20.0f, 19000.0f, modB1Freq.getCurrentValue()); 
        float g1 = juce::jlimit(-24.0f, 24.0f, modB1Gain.getCurrentValue());
        float q1 = juce::jlimit(0.1f, 10.0f, modB1Q.getCurrentValue());

        auto& filter1 = filterChain.get<0>();
        if (filter1.state != nullptr) {
            switch (b1Type) {
                case HighPass: *filter1.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, f1, q1); break;
                case LowShelf: *filter1.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, f1, q1, juce::Decibels::decibelsToGain(g1)); break;
                case Bell:     *filter1.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, f1, q1, juce::Decibels::decibelsToGain(g1)); break;
                default:       *filter1.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, f1, q1, 1.0f); break;
            }
        }

        float f2 = juce::jlimit(20.0f, 19000.0f, modB2Freq.getCurrentValue());
        float g2 = juce::jlimit(-24.0f, 24.0f, modB2Gain.getCurrentValue());
        float q2 = juce::jlimit(0.1f, 10.0f, modB2Q.getCurrentValue());

        auto& filter2 = filterChain.get<1>();
        if (filter2.state != nullptr) {
            switch (b2Type) {
                case LowPass:   *filter2.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, f2, q2); break;
                case HighShelf: *filter2.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf(sampleRate, f2, q2, juce::Decibels::decibelsToGain(g2)); break;
                case Bell:      *filter2.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, f2, q2, juce::Decibels::decibelsToGain(g2)); break;
                case HighPass:  *filter2.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRate, f2, q2); break;
                case LowShelf:  *filter2.state = *juce::dsp::IIR::Coefficients<float>::makeLowShelf(sampleRate, f2, q2, juce::Decibels::decibelsToGain(g2)); break;
                default:        *filter2.state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, f2, q2, 1.0f); break;
            }
        }
    }

    double sampleRate = 44100.0;
    std::atomic<bool> isPrepared { false };
    
    juce::dsp::ProcessorChain<
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>,
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>>
    > filterChain;

    SimpleAnalyzer analyzer;
};
