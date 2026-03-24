#pragma once
#include <JuceHeader.h>
#include <map>

// --- EL ESCUDO CONTRA WINDOWS ---
class SilentWindowButton : public juce::TextButton {
public:
    SilentWindowButton(const juce::String& name) : juce::TextButton(name) {}
    void setTooltip(const juce::String&) override {}
};

class CustomTheme : public juce::LookAndFeel_V4
{
public:
    CustomTheme();

    void drawDocumentWindowTitleBar(juce::DocumentWindow& window, juce::Graphics& g,
        int w, int h, int titleSpaceX, int titleSpaceW,
        const juce::Image* icon, bool drawTitleTextOnLeft) override;

    juce::Button* createDocumentWindowButton(int buttonType) override;

    // ==============================================================================
    // SISTEMA DE SKINS GLOBAL (PNG FILMSTRIPS + XML)
    // ==============================================================================
    void loadSkinFromFolder(const juce::File& skinFolder);

    juce::Colour getSkinColor(const juce::String& colorName, juce::Colour fallbackColor);

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
        const juce::Colour& backgroundColour,
        bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

private:
    // Punteros en RAM para las Tiras PNG (Filmstrips)
    juce::Image imgKnobFilmstrip;
    juce::Image imgFaderTrack;
    juce::Image imgFaderThumb;

    std::map<juce::String, juce::Colour> skinColors;
};