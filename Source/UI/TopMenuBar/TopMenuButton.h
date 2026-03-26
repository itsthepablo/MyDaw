#pragma once
#include <JuceHeader.h>

class TopMenuButton : public juce::Button {
public:
    TopMenuButton(const juce::String& name) : Button(name) {}

    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override {
        if (isButtonDown || isMouseOverButton) {
            g.fillAll(juce::Colours::white.withAlpha(0.08f));
        }

        g.setColour(isMouseOverButton ? juce::Colours::white : juce::Colour(170, 175, 180));

        juce::Font font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::plain);
        g.setFont(font);
        g.drawFittedText(getName(), getLocalBounds(), juce::Justification::centred, 1, 0.8f);
    }
};