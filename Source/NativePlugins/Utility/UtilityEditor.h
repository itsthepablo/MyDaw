#pragma once
#include <JuceHeader.h>
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

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Slider gainSlider;
    juce::Slider panSlider;
    std::atomic<float>& gain;
    std::atomic<float>& pan;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UtilityEditor)
};
