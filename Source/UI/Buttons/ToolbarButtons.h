#pragma once
#include <JuceHeader.h>

class ToolbarButtons : public juce::Component {
public:
    ToolbarButtons() {
        addAndMakeVisible(showMixerBtn);
        showMixerBtn.setButtonText("MIXER");
        showMixerBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);

        addAndMakeVisible(showEffectsBtn);
        showEffectsBtn.setButtonText("FX");
        showEffectsBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    }

    void resized() override {
        auto area = getLocalBounds();
        // Los botones se alinean a la derecha uno tras otro
        showMixerBtn.setBounds(area.removeFromRight(80).reduced(2));
        showEffectsBtn.setBounds(area.removeFromRight(80).reduced(2));
    }

    juce::TextButton showMixerBtn;
    juce::TextButton showEffectsBtn;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolbarButtons)
};