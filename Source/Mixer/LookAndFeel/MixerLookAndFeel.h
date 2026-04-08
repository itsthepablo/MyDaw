#pragma once
#include <JuceHeader.h>
#include "MixerLevelMeterLF.h"
#include "MixerColours.h"

// ==============================================================================
// 1. LOOK AND FEEL PROFESIONAL PARA EL MIXER
// Declaraciones — La implementación está en MixerLookAndFeel.cpp
// ==============================================================================
class MixerLookAndFeel : public juce::LookAndFeel_V4, public LevelMeterLookAndFeel {
public:
    MixerLookAndFeel();
    
    void drawScrollbarButton(juce::Graphics&, juce::ScrollBar&, int, int, int, bool, bool, bool) override;
    
    void drawScrollbar(juce::Graphics& g, juce::ScrollBar& s, int x, int y, int width, int height,
                       bool isVertical, int thumbStart, int thumbSize, bool isMouseOver, bool isMouseDown) override;

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    // --- IMPLEMENTACIÓN DEL MEDIDOR DE PICOS (Desde MixerLevelMeterLF.h) ---
    void drawLevelMeter(juce::Graphics& g, 
                        juce::Rectangle<float> bounds, 
                        float levelL, float levelR, 
                        float peakL, float peakR, 
                        bool isClipping, bool isHorizontal) override;
};