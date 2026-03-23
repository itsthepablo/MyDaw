#pragma once
#include <JuceHeader.h>
#include "BaseEffect.h"
#include <atomic>

// ==============================================================================
// INTERFAZ GRÁFICA INCRUSTADA
// ==============================================================================
class UtilityEditor : public juce::Component {
public:
    UtilityEditor(std::atomic<float>& g, std::atomic<float>& p) : gain(g), pan(p) {
        addAndMakeVisible(gainSlider);
        gainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        gainSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        gainSlider.setRange(0.0, 2.0, 0.01);
        gainSlider.setValue(gain.load());
        gainSlider.onValueChange = [this] { gain.store((float)gainSlider.getValue()); };

        addAndMakeVisible(panSlider);
        panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panSlider.setRange(-1.0, 1.0, 0.01);
        panSlider.setValue(pan.load());
        panSlider.onValueChange = [this] { pan.store((float)panSlider.getValue()); };
    }

    void paint(juce::Graphics& g) override {
        auto area = getLocalBounds();

        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::Font("Sans-Serif", 11.0f, juce::Font::bold));

        int halfW = area.getWidth() / 2;
        g.drawText("GAIN", 0, area.getBottom() - 20, halfW, 15, juce::Justification::centred);
        g.drawText("PAN", halfW, area.getBottom() - 20, halfW, 15, juce::Justification::centred);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(5);
        area.removeFromTop(5);
        area.removeFromBottom(25);

        int halfW = area.getWidth() / 2;
        gainSlider.setBounds(area.removeFromLeft(halfW).reduced(2));
        panSlider.setBounds(area.reduced(2));
    }

private:
    juce::Slider gainSlider;
    juce::Slider panSlider;
    std::atomic<float>& gain;
    std::atomic<float>& pan;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UtilityEditor)
};

// ==============================================================================
// MOTOR DSP DEL PLUGIN NATIVO (True Stereo Panner Lineal)
// ==============================================================================
class UtilityPlugin : public BaseEffect {
public:
    UtilityPlugin() {
        gain.store(1.0f);
        pan.store(0.0f);
        editor = std::make_unique<UtilityEditor>(gain, pan);
    }

    bool isLoaded() const override { return true; }
    juce::String getLoadedPluginName() const override { return "Utility"; }
    bool isBypassed() const override { return bypassed; }
    void setBypassed(bool b) override { bypassed = b; }

    void showWindow() override {
        // Los plugins nativos no usan ventana flotante
    }

    void prepareToPlay(double, int) override {
        // Inicialización de DSP interno si fuera necesario
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        if (bypassed) return;

        float p = pan.load();   // Rango: -1.0 (Izquierda) a 1.0 (Derecha)
        float g = gain.load();  // Rango: 0.0 a 2.0

        if (buffer.getNumChannels() >= 2) {
            float* left = buffer.getWritePointer(0);
            float* right = buffer.getWritePointer(1);

            // Factor de compensación (Normalización) para evitar Clipping al sumar canales
            // Si p=0 (centro), norm=1.0 (0dB de ganancia). 
            // Si p=1 (extremo), norm=0.707 (-3dB de ganancia) para compensar la suma de L+R.
            float norm = 1.0f / std::sqrt(1.0f + std::abs(p));

            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float inL = left[i];
                float inR = right[i];

                float outL = inL;
                float outR = inR;

                if (p < 0.0f) {
                    // Paneando Izquierda (-1 a 0):
                    // El altavoz Izquierdo conserva su sonido y absorbe la energía del Derecho
                    float panVal = -p; // Convertimos a positivo para el cálculo
                    outL = inL + (inR * panVal);
                    outR = inR * (1.0f - panVal); // A -1.0 absoluto, outR se multiplica por 0
                }
                else if (p > 0.0f) {
                    // Paneando Derecha (0 a 1):
                    // El altavoz Derecho conserva su sonido y absorbe la energía del Izquierdo
                    outL = inL * (1.0f - p); // A 1.0 absoluto, outL se multiplica por 0
                    outR = inR + (inL * p);
                }

                // Aplicamos la normalización anti-clip y la ganancia maestra del Utility
                left[i] = outL * norm * g;
                right[i] = outR * norm * g;
            }
        }
        else if (buffer.getNumChannels() == 1) {
            // Protección para pistas Mono
            buffer.applyGain(0, 0, buffer.getNumSamples(), g);
        }
    }

    bool isNative() const override { return true; }
    juce::Component* getCustomEditor() override { return editor.get(); }

private:
    bool bypassed = false;
    std::atomic<float> gain;
    std::atomic<float> pan;
    std::unique_ptr<UtilityEditor> editor;
};