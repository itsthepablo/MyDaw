#pragma once
#include <JuceHeader.h>

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
            if (isButtonDown) g.fillAll(juce::Colours::white.withAlpha(0.2f));
            else if (isMouseOverButton) g.fillAll(juce::Colours::white.withAlpha(0.1f));
        }
        g.setColour(isMouseOverButton && type == Close ? juce::Colours::white : juce::Colour(170, 175, 180));
        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float w = 10.0f;
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