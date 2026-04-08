#pragma once
#include <JuceHeader.h>

// ==============================================================================
// 1. LOOK AND FEEL PROFESIONAL PARA EL MIXER
// Esta clase ahora vive en su propio archivo. Solo dibuja, no procesa audio.
// ==============================================================================
class MixerLookAndFeel : public juce::LookAndFeel_V4 {
public:
    MixerLookAndFeel() {
        setColour(juce::Slider::thumbColourId, juce::Colour(200, 200, 200));
        setColour(juce::Slider::trackColourId, juce::Colour(30, 30, 30));
        setColour(juce::ScrollBar::thumbColourId, juce::Colour(60, 60, 60));
        setColour(juce::ScrollBar::backgroundColourId, juce::Colours::transparentBlack);
    }
    
    void drawScrollbarButton(juce::Graphics&, juce::ScrollBar&, int, int, int, bool, bool, bool) override {}
    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& s, int x, int y, int width, int height,
                       bool isVertical, int thumbStart, int thumbSize, bool isMouseOver, bool isMouseDown) override {
        juce::ignoreUnused(isMouseOver, isMouseDown);
        auto b = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
        g.setColour(s.findColour(juce::ScrollBar::backgroundColourId));
        g.fillRoundedRectangle(b, 2.0f);
        if (thumbSize > 0) {
            g.setColour(s.findColour(juce::ScrollBar::thumbColourId).withAlpha(isMouseOver ? 0.8f : 0.4f));
            if (isVertical) g.fillRoundedRectangle(b.getX() + 1, (float)thumbStart, b.getWidth() - 2, (float)thumbSize, 2.0f);
            else g.fillRoundedRectangle((float)thumbStart, b.getY() + 1, (float)thumbSize, b.getHeight() - 2, 2.0f);
        }
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override {
        auto outline = juce::Colour(50, 50, 50);
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toX = bounds.getCentreX();
        auto toY = bounds.getCentreY();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(juce::Colour(25, 25, 25));
        g.fillEllipse(toX - radius, toY - radius, radius * 2.0f, radius * 2.0f);

        g.setColour(outline);
        g.drawEllipse(toX - radius, toY - radius, radius * 2.0f, radius * 2.0f, 1.5f);

        juce::Path p;
        auto pointerLength = radius * 0.8f;
        auto pointerThickness = 2.5f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(toX, toY));

        g.setColour(juce::Colours::orange.withAlpha(0.9f));
        g.fillPath(p);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override {

        auto trackWidth = 4.0f;
        auto isVertical = style == juce::Slider::LinearVertical;

        g.setColour(juce::Colour(15, 15, 15));
        if (isVertical)
            g.fillRoundedRectangle(x + width * 0.5f - trackWidth * 0.5f, y, trackWidth, height, 2.0f);
        else
            g.fillRoundedRectangle(x, y + height * 0.5f - trackWidth * 0.5f, width, trackWidth, 2.0f);

        auto handleWidth = isVertical ? width * 0.8f : 30.0f;
        auto handleHeight = isVertical ? 20.0f : height * 0.8f;

        juce::Rectangle<float> handle;
        if (isVertical)
            handle = { x + width * 0.5f - handleWidth * 0.5f, sliderPos - handleHeight * 0.5f, handleWidth, handleHeight };
        else
            handle = { sliderPos - handleWidth * 0.5f, y + height * 0.5f - handleHeight * 0.5f, handleWidth, handleHeight };

        g.setColour(juce::Colour(60, 60, 65));
        g.fillRoundedRectangle(handle, 3.0f);

        g.setColour(juce::Colours::orange);
        if (isVertical)
            g.fillRect(handle.getX(), handle.getCentreY() - 1.0f, handle.getWidth(), 2.0f);
        else
            g.fillRect(handle.getCentreX() - 1.0f, handle.getY(), 2.0f, handle.getHeight());

        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawRoundedRectangle(handle, 3.0f, 1.0f);
    }
};