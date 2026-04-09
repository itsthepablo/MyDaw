#pragma once
#include <JuceHeader.h>

/**
 * AudioClipLookAndFeel: Define la estética de los clips de Audio.
 * Centraliza colores, fuentes y estilos de dibujo.
 */
class AudioClipLookAndFeel : public juce::LookAndFeel_V4 {
public:
    AudioClipLookAndFeel() {
        setColour(headerBackgroundId, juce::Colour(0x60000000));
        setColour(waveformColorId, juce::Colours::white);
        setColour(selectedOutlineId, juce::Colours::yellow);
    }

    virtual void drawAudioClipBackground(juce::Graphics& g, juce::Rectangle<float> area, juce::Colour trackColor, bool isSelected) {
        g.setColour(trackColor.darker(0.8f).withAlpha(1.0f));
        g.fillRoundedRectangle(area, 5.0f);

        if (isSelected) {
            g.setColour(findColour(selectedOutlineId));
            g.drawRoundedRectangle(area, 5.0f, 1.5f);
        }
    }

    virtual void drawAudioClipHeader(juce::Graphics& g, juce::Rectangle<float> area, const juce::String& name, juce::Colour trackColor) {
        g.setColour(trackColor.darker(0.8f).withAlpha(0.4f));
        g.fillRoundedRectangle(area, 5.0f);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(" " + name, area.reduced(3.0f, 0.0f).toNearestInt(),
                   juce::Justification::centredLeft, true);
    }

    virtual juce::Colour getWaveformColor(juce::Colour trackColor) {
        return trackColor.brighter(0.1f);
    }

    enum ColourIDs {
        headerBackgroundId = 0x2001001,
        waveformColorId = 0x2001002,
        selectedOutlineId = 0x2001003
    };
};
