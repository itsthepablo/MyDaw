#pragma once
#include <JuceHeader.h>
#include "../../../Theme/CustomTheme.h"

class SvgToggleButton : public juce::Button {
public:
    SvgToggleButton(juce::String offIconName, juce::String onIconName, 
                    juce::Colour offCol = juce::Colours::white, 
                    juce::Colour onCol = juce::Colours::limegreen) 
        : Button("SvgToggle"), 
          offName(offIconName), onName(onIconName),
          offColor(offCol), onColor(onCol)
    {
        setClickingTogglesState(true);
    }

    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override {
        auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel());
        if (!theme) return;

        bool isOn = getToggleState();
        juce::Colour currentColor = isOn ? onColor : offColor;
        
        // SIN CACHÉ PARA EVITAR INVERSIONES (Regla de Emergencia)
        auto name = isOn ? onName : offName;
        auto icon = theme->getIcon(name, currentColor);

        if (icon != nullptr) {
            auto totalBounds = getLocalBounds().toFloat().reduced(2.0f);
            float opacity = isButtonDown ? 0.7f : (isMouseOverButton ? 1.0f : 0.9f);
            
            // Dibuja respetando la composición y proporción original del SVG
            icon->drawWithin(g, totalBounds, juce::RectanglePlacement::centred, opacity);
        }
    }

private:
    juce::String offName, onName;
    juce::Colour offColor, onColor;
    
    // Persistencia para evitar recalcular en cada frame
    std::unique_ptr<juce::Drawable> currentIcon;
    bool lastState = false;
    juce::Colour lastColor;
};
