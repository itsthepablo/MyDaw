#pragma once
#include <JuceHeader.h>

// ==============================================================================
// CAJA FLOTANTE UNIVERSAL ESTILO PLUGIN
// Muestra el valor en dB, Paneo (L/R) o % horizontalmente
// ==============================================================================
class FloatingValueBox : public juce::Component {
public:
    FloatingValueBox() {
        setAlwaysOnTop(true);
        setOpaque(true);
        
        // Color de borde por defecto estilo FL Studio (Naranja/Dorado)
        borderColor = juce::Colour(200, 130, 30);
    }

    void setText(const juce::String& t) {
        if (text != t) {
            text = t;
            repaint();
        }
    }

    void setBorderColor(juce::Colour c) {
        if (borderColor != c) {
            borderColor = c;
            repaint();
        }
    }

    void paint(juce::Graphics& g) override {
        // Fondo muy oscuro (Idéntico a FL y FabFilter)
        g.fillAll(juce::Colour(15, 17, 20)); 
        
        // Borde dorado naranja
        g.setColour(borderColor);
        g.drawRect(getLocalBounds(), 1);
        
        // Texto horizontal blanco y claro (sin rotar, legible instantáneamente)
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText(text, getLocalBounds(), juce::Justification::centred, false);
    }

private:
    juce::String text;
    juce::Colour borderColor;
};