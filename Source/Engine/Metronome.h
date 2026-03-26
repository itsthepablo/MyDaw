#pragma once
#include <JuceHeader.h>
#include "AudioClock.h"
#include <cmath>

class Metronome {
public:
    Metronome() = default;

    void prepare(double newSampleRate) {
        sampleRate = newSampleRate;
        phase = 0.0;
        envelope = 0.0;
        wasStopped = true;
    }

    void setEnabled(bool shouldBeEnabled) {
        isEnabled = shouldBeEnabled;
    }

    void process(juce::AudioBuffer<float>& buffer, int numSamples, const AudioClock& clock, double bpm) {
        if (!isEnabled) {
            wasStopped = !clock.isPlayingInternal;
            return;
        }

        if (!clock.isPlayingInternal) {
            wasStopped = true;
            return;
        }

        // Matemática de tu DAW: 80 píxeles = 1 beat
        double pixelsPerSec = (bpm / 60.0) * 80.0;
        double pixelsPerSample = pixelsPerSec / sampleRate;
        double decayRate = 1.0 / (sampleRate * 0.05); // Decaimiento de 50ms para un "click" corto

        float* left = buffer.getWritePointer(0);
        float* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

        double ph = clock.currentPh;

        for (int i = 0; i < numSamples; ++i) {
            double currentBeat = ph / 80.0;
            double nextPh = ph + pixelsPerSample;
            double nextBeat = nextPh / 80.0;

            // Detectamos si cruzamos un número entero (cayó el beat)
            bool crossed = (std::floor(currentBeat) != std::floor(nextBeat));
            
            // Disparar click inmediatamente si acabamos de darle a Play desde 0
            if (wasStopped && i == 0) {
                crossed = true;
            }

            if (crossed) {
                envelope = 1.0;
                int beatIndex = (int)std::floor(nextBeat);
                isDownbeat = (beatIndex % 4 == 0); // Frecuencia alta cada 4 tiempos
                phase = 0.0;
            }

            if (envelope > 0.0) {
                double freq = isDownbeat ? 1000.0 : 500.0;
                double inc = (juce::MathConstants<double>::twoPi * freq) / sampleRate;
                phase += inc;
                if (phase > juce::MathConstants<double>::twoPi) phase -= juce::MathConstants<double>::twoPi;

                // Generar sonido a la mitad de volumen (0.5)
                float sample = (float)(std::sin(phase) * envelope * 0.5);

                left[i] += sample;
                if (right) right[i] += sample;

                envelope -= decayRate;
                if (envelope < 0.0) envelope = 0.0;
            }

            ph = nextPh;
        }
        wasStopped = false;
    }

private:
    double sampleRate = 44100.0;
    double phase = 0.0;
    double envelope = 0.0;
    bool isEnabled = false;
    bool isDownbeat = true;
    bool wasStopped = true;
};