#pragma once
#include <JuceHeader.h>
#include "../BaseEffect.h"
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

    void showWindow(TrackContainer* container = nullptr) override {
        // Los plugins nativos no usan ventana flotante por ahora
    }

    void prepareToPlay(double, int) override {
        // Inicialización de DSP interno si fuera necesario
    }

    void reset() override {
        // No hay colas de memoria que resetear
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
        if (bypassed) return;

        float p = pan.load();
        float g = gain.load();

        if (buffer.getNumChannels() >= 2) {
            float* left = buffer.getWritePointer(0);
            float* right = buffer.getWritePointer(1);

            float norm = 1.0f / std::sqrt(1.0f + std::abs(p));

            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                float inL = left[i];
                float inR = right[i];

                float outL = inL;
                float outR = inR;

                if (p < 0.0f) {
                    float panVal = -p;
                    outL = inL + (inR * panVal);
                    outR = inR * (1.0f - panVal);
                }
                else if (p > 0.0f) {
                    outL = inL * (1.0f - p);
                    outR = inR + (inL * p);
                }

                left[i] = outL * norm * g;
                right[i] = outR * norm * g;
            }
        }
        else if (buffer.getNumChannels() == 1) {
            buffer.applyGain(0, 0, buffer.getNumSamples(), g);
        }
    }

    // --- EL UTILITY ES MATEMÁTICA PURA, NO TIENE LATENCIA ---
    int getLatencySamples() const override { return 0; }

    bool isNative() const override { return true; }
    juce::Component* getCustomEditor() override { return editor.get(); }

private:
    bool bypassed = false;
    std::atomic<float> gain;
    std::atomic<float> pan;
    std::unique_ptr<UtilityEditor> editor;
};