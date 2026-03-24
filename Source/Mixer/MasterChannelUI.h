#pragma once
#include <JuceHeader.h>
#include "MixerComponent.h"
#include "MixerChannelUI.h" // Para reutilizar el LookAndFeel por ahora

// ==============================================================================
// MASTER CHANNEL UI
// ==============================================================================
class MasterChannelUI : public juce::Component {
public:
    MasterChannelUI(MixerComponent& m) : mixer(m) {
        setLookAndFeel(&globalLAF);

        volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
        volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        volumeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::red);
        volumeSlider.setRange(0.0, 1.2);
        volumeSlider.setValue(mixer.getMasterVolume());
        volumeSlider.onValueChange = [this] { mixer.setMasterVolume((float)volumeSlider.getValue()); };
        addAndMakeVisible(volumeSlider);

        trackName.setText("MASTER", juce::dontSendNotification);
        trackName.setJustificationType(juce::Justification::centred);
        trackName.setColour(juce::Label::textColourId, juce::Colours::white);
        trackName.setFont(juce::Font(14.0f, juce::Font::bold));
        addAndMakeVisible(trackName);
    }

    ~MasterChannelUI() override { setLookAndFeel(nullptr); }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(45, 25, 25));
        g.setColour(juce::Colour(20, 22, 25));
        g.drawRect(getLocalBounds(), 2);
    }

    void resized() override {
        float scale = getHeight() / 600.0f;

        int sliderW = juce::roundToInt(60 * scale);
        int sliderH = juce::roundToInt(370 * scale);
        int sliderX = (getWidth() - sliderW) / 2;
        int sliderY = juce::roundToInt(220 * scale);

        volumeSlider.setBounds(sliderX, sliderY, sliderW, sliderH);
        trackName.setBounds(0, getHeight() - juce::roundToInt(25 * scale), getWidth(), juce::roundToInt(20 * scale));
    }

private:
    MixerComponent& mixer;
    ModernDialLookAndFeel globalLAF;
    juce::Slider volumeSlider;
    juce::Label trackName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterChannelUI)
};