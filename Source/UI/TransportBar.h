#pragma once
#include <JuceHeader.h>

class TransportBar : public juce::Component {
public:
    TransportBar() {
        // Botón Play/Stop
        addAndMakeVisible(playBtn);
        playBtn.setButtonText("PLAY");
        playBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::green);

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
        
        // Diseño de la barra: Botón a la izquierda, BPM al lado
        playBtn.setBounds(area.removeFromLeft(100).reduced(5));
        
        auto bpmArea = area.removeFromLeft(100);
        bpmLabel.setBounds(bpmArea.removeFromTop(18));
        bpmSlider.setBounds(bpmArea.reduced(2));
    }

    juce::TextButton playBtn;
    juce::Slider bpmSlider;
    juce::Label bpmLabel;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};