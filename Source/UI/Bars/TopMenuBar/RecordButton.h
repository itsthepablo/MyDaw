#pragma once
#include <JuceHeader.h>

class RecordButton : public juce::Button {
public:
    RecordButton() : Button("RecordBtn") {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override {
        auto bounds = getLocalBounds().toFloat().reduced(10.0f);

        if (isButtonDown) {
            bounds.reduce(1.0f, 1.0f);
        }

        if (getToggleState()) {
            g.setColour(juce::Colour(255, 40, 40));
        } else {
            g.setColour(isMouseOverButton ? juce::Colour(100, 105, 110) : juce::Colour(70, 75, 80));
        }

        g.fillEllipse(bounds);
        
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawEllipse(bounds, 1.0f);
    }
};