#pragma once
#include <JuceHeader.h>

// --- EL ESCUDO CONTRA WINDOWS ---
// Creamos un botón que ignora silenciosamente cualquier intento de ponerle un texto flotante (Tooltip)
class SilentWindowButton : public juce::TextButton {
public:
    SilentWindowButton(const juce::String& name) : juce::TextButton(name) {}

    // ¡Al dejar esta función vacía, es IMPOSIBLE que Windows muestre la burbuja amarilla!
    void setTooltip(const juce::String&) override {}
};

class CustomTheme : public juce::LookAndFeel_V4
{
public:
    CustomTheme();

    void drawDocumentWindowTitleBar(juce::DocumentWindow& window, juce::Graphics& g,
        int w, int h, int titleSpaceX, int titleSpaceW,
        const juce::Image* icon, bool drawTitleTextOnLeft) override;

    // Aquí inyectaremos nuestros botones blindados
    juce::Button* createDocumentWindowButton(int buttonType) override;
};