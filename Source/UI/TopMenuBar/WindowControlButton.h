#pragma once
#include <JuceHeader.h>
#include "../../Theme/CustomTheme.h"

class WindowControlButton : public juce::Button {
public:
    enum Type { Minimize, Maximize, Close };

    WindowControlButton(Type t) : Button("WinCtrlBtn"), type(t) {
        setTooltip(getTooltipForType(t));
    }

    juce::String getTooltipForType(Type t) {
        if (t == Minimize) return "Minimizar ventana";
        if (t == Maximize) return "Maximizar / Restaurar ventana";
        return "Cerrar DAW";
    }

    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override {
        auto bounds = getLocalBounds().toFloat();
        if (type == Close) {
            if (isButtonDown) g.fillAll(juce::Colour(255, 100, 100));
            else if (isMouseOverButton) g.fillAll(juce::Colour(232, 17, 35));
        }
        else {
            if (isButtonDown) g.fillAll(juce::Colour(31, 132, 255).darker(0.2f));
            else if (isMouseOverButton) g.fillAll(juce::Colour(31, 132, 255));
        }

        juce::Colour iconColor = isMouseOverButton ? juce::Colours::black : juce::Colour(170, 175, 180);
        g.setColour(iconColor);

        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float w = 10.0f;

        // --- RENDERIZADO SVG (Si existe el Path) ---
        if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
            juce::String iconId = (type == Close ? "CLOSE" : (type == Maximize ? "MAX" : "MIN"));
            if (auto icon = theme->getIcon(iconId, iconColor)) {
                icon->drawWithin(g, bounds.reduced(bounds.getWidth() * 0.3f), juce::RectanglePlacement::centred, 1.0f);
                return;
            }
        }

        // --- FALLBACK: DIBUJO MANUAL ---
        if (type == Minimize) g.drawHorizontalLine((int)(cy + 4), cx - w / 2, cx + w / 2 + 1);
        else if (type == Maximize) g.drawRect(cx - w / 2, cy - w / 2, w, w, 1.2f);
        else if (type == Close) {
            g.drawLine(cx - w / 2, cy - w / 2, cx + w / 2, cy + w / 2, 1.2f);
            g.drawLine(cx - w / 2, cy + w / 2, cx + w / 2, cy - w / 2, 1.2f);
        }
    }
private:
    Type type;
};