#pragma once
#include <JuceHeader.h>

/**
 * Identidad visual para el módulo de ruteo.
 * Centraliza colores y estilos para Sends y Sidechain.
 */
class RoutingLookAndFeel : public juce::LookAndFeel_V4 {
public:
    RoutingLookAndFeel() {
        // Colores base de ruteo
        setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        setColour(juce::Slider::trackColourId, juce::Colour(40, 45, 50));
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(35, 38, 42));
        setColour(juce::ComboBox::outlineColourId, juce::Colours::white.withAlpha(0.1f));
        setColour(juce::TextButton::buttonColourId, juce::Colour(50, 55, 60));
    }

    // --- COLORES PALETA ---
    static juce::Colour getSendOrange()      { return juce::Colours::orange; }
    static juce::Colour getSidechainGold()   { return juce::Colour(255, 200, 50); } // Dorado/Ámbar
    static juce::Colour getBackgroundDark()  { return juce::Colour(25, 28, 32); }
    static juce::Colour getSlotBackground()  { return juce::Colour(55, 60, 65); }

    // --- SOBREESCRITURA DE DIBUJO ---
    
    // Corregido: drawLinearSliderThumb es el método correcto para Sliders de barra/línea
    void drawLinearSliderThumb (juce::Graphics& g, int x, int y, int width, int height,
                                float sliderPos, float minSliderPos, float maxSliderPos,
                                const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto area = juce::Rectangle<float>(x, y, width, height).reduced(2.0f);
        g.setColour(slider.findColour(juce::Slider::thumbColourId));
        g.fillRoundedRectangle(area, 2.0f);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override {
        auto cornerSize = 4.0f;
        auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);

        auto baseColour = backgroundColour.withMultipliedSaturation (shouldDrawButtonAsHighlighted ? 1.3f : 1.0f)
                                           .withMultipliedBrightness (shouldDrawButtonAsDown ? 0.8f : 1.0f);

        g.setColour (baseColour);
        g.fillRoundedRectangle (bounds, cornerSize);

        g.setColour (button.findColour (juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle (bounds, cornerSize, 1.0f);
    }

    void drawLabel(juce::Graphics& g, juce::Label& label) override {
        g.fillAll(label.findColour(juce::Label::backgroundColourId));
        
        auto alpha = label.isEnabled() ? 1.0f : 0.5f;
        const juce::Font font(label.getFont());
        
        g.setColour(label.findColour(juce::Label::textColourId).withMultipliedAlpha(alpha));
        g.setFont(font);
        
        auto textArea = label.getBorderSize().subtractedFrom(label.getLocalBounds());
        g.drawText(label.getText(), textArea, label.getJustificationType(), true);
    }
};
