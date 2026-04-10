#pragma once
#include <JuceHeader.h>
#include "../../../Theme/CustomTheme.h"

class SvgButton : public juce::Button {
public:
    SvgButton(const juce::String& iconName) : Button(iconName), name(iconName) {}

    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override {
        auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel());
        if (!theme) return;

        // Color solicitado: Blanco (con feedback de interacción)
        juce::Colour iconColor = juce::Colours::white;
        
        // --- CACHÉ Y NORMALIZACIÓN (Senior Architect Pattern) ---
        if (currentIcon == nullptr || lastColor != iconColor) {
            lastColor = iconColor;
            currentIcon = theme->getIcon(name, lastColor);
        }

        if (currentIcon != nullptr) {
            auto totalBounds = getLocalBounds().toFloat().reduced(2.0f);
            float opacity = isButtonDown ? 0.7f : (isMouseOverButton ? 1.0f : 0.9f);
            
            // Dibuja respetando la composición y proporción original del SVG
            currentIcon->drawWithin(g, totalBounds, juce::RectanglePlacement::centred, opacity);
        }
    }

private:
    juce::String name;
    std::unique_ptr<juce::Drawable> currentIcon;
    juce::Colour lastColor;
};
