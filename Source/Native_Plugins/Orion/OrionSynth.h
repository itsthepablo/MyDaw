#pragma once
#include <JuceHeader.h>

// ==============================================================================
class OrionSound : public juce::SynthesiserSound {
public:
    OrionSound() = default;
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};

// ==============================================================================
class OrionVoice : public juce::SynthesiserVoice {
public:
    OrionVoice() {
        lowPassFilter.setMode(juce::dsp::LadderFilterMode::LPF12);
        
        // Configuración ADSR por defecto para evitar el silencio inicial
        juce::ADSR::Parameters p;
        p.attack = 0.01f; p.decay = 0.1f; p.sustain = 1.0f; p.release = 0.2f;
        adsr.setParameters(p);
    }

    bool canPlaySound(juce::SynthesiserSound* sound) override {
        return dynamic_cast<OrionSound*>(sound) != nullptr;
    }

    void startNote(int midiNoteNumber, float velocity, juce::SynthesiserSound*, int) override {
        level = velocity * 0.5f;
        auto cyclesPerSample = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber) / getSampleRate();
        phaseDelta = cyclesPerSample * 2.0 * juce::MathConstants<double>::pi;
        phase = 0.0;
        adsr.noteOn();
    }

    void stopNote(float, bool allowTail) override {
        adsr.noteOff();
        if (!allowTail) clearCurrentNote();
    }

    void pitchWheelMoved(int) override {}
    void controllerMoved(int, int) override {}

    void prepare(const juce::dsp::ProcessSpec& spec) {
        lowPassFilter.prepare(spec);
        adsr.setSampleRate(spec.sampleRate);
        
        synthBuffer.setSize(spec.numChannels, spec.maximumBlockSize, false, false, true);
        
        smoothedCutoff.reset(spec.sampleRate, 0.02);
        smoothedRes.reset(spec.sampleRate, 0.02);
        smoothedCutoff.setCurrentAndTargetValue(2000.0f);
        smoothedRes.setCurrentAndTargetValue(0.1f);
    }

    void updateParams(float a, float d, float s, float r, float cutoff, float res, int type1, int type2) {
        juce::ADSR::Parameters params;
        params.attack = a; params.decay = d; params.sustain = s; params.release = r;
        adsr.setParameters(params);
        smoothedCutoff.setTargetValue(cutoff);
        smoothedRes.setTargetValue(res);
        oscType = type1;
    }

    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override {
        if (!isVoiceActive()) return;

        synthBuffer.setSize(outputBuffer.getNumChannels(), numSamples, false, false, true);
        synthBuffer.clear();

        auto* outL = synthBuffer.getWritePointer(0);
        auto* outR = synthBuffer.getNumChannels() > 1 ? synthBuffer.getWritePointer(1) : nullptr;

        for (int s = 0; s < numSamples; ++s) {
            float sample = 0.0f;
            
            // Generación manual de onda (Saw simple)
            if (oscType == 1) sample = (float)(phase / juce::MathConstants<double>::pi);
            else sample = (float)std::sin(phase);

            if (outL) outL[s] = sample * level;
            if (outR) outR[s] = sample * level;

            phase += phaseDelta;
            if (phase > juce::MathConstants<double>::pi) phase -= juce::MathConstants<double>::twoPi;
        }

        // Aplicar Filtro y ADSR
        juce::dsp::AudioBlock<float> block(synthBuffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        lowPassFilter.setCutoffFrequencyHz(smoothedCutoff.getNextValue());
        lowPassFilter.setResonance(smoothedRes.getNextValue());
        lowPassFilter.process(context);
        
        adsr.applyEnvelopeToBuffer(synthBuffer, 0, numSamples);

        for (int ch = 0; ch < outputBuffer.getNumChannels(); ++ch) {
            outputBuffer.addFrom(ch, startSample, synthBuffer, ch, 0, numSamples);
        }

        if (!adsr.isActive()) clearCurrentNote();
    }

private:
    double phase = 0.0;
    double phaseDelta = 0.0;
    float level = 0.0f;
    int oscType = 1;

    juce::dsp::LadderFilter<float> lowPassFilter;
    juce::ADSR adsr;
    juce::AudioBuffer<float> synthBuffer;

    juce::LinearSmoothedValue<float> smoothedCutoff;
    juce::LinearSmoothedValue<float> smoothedRes;
};
