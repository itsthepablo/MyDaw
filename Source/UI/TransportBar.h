#pragma once
#include <JuceHeader.h>

class TransportBar : public juce::Component {
public:
    TransportBar() {
        // Slider de BPM
        addAndMakeVisible(bpmSlider);
        bpmSlider.setRange(40.0, 240.0, 0.1);
        bpmSlider.setValue(120.0);
        bpmSlider.setSliderStyle(juce::Slider::LinearBarVertical);
        bpmSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);

        // Etiqueta de BPM
        addAndMakeVisible(bpmLabel);
        bpmLabel.setText("120.0 BPM", juce::dontSendNotification);
        bpmLabel.setJustificationType(juce::Justification::centred);
        bpmLabel.setFont(juce::Font(14.0f, juce::Font::bold));
    }

    void resized() override {
        auto area = getLocalBounds();

        // Diseño de la barra para el slider y BPM (Los botones migraron a TopMenuBar)
        auto bpmArea = area.removeFromLeft(100);
        bpmLabel.setBounds(bpmArea.removeFromTop(18));
        bpmSlider.setBounds(bpmArea.reduced(2));
    }

    juce::Slider bpmSlider;
    juce::Label bpmLabel;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};