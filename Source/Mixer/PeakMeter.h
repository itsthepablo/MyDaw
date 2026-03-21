#pragma once
#include <JuceHeader.h>

class PeakMeter : public juce::Component, public juce::Timer {
public:
    PeakMeter() {
        startTimerHz(60); // 60 FPS para que la caída de la luz sea súper fluida
    }

    ~PeakMeter() override {
        stopTimer();
    }

    // El motor de audio llamará a esta función enviando el volumen actual (0.0 a 1.0)
    void setLevel(float newLevel) {
        if (newLevel > level) {
            level = newLevel; // Sube al instante
        }
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();

        // 1. Fondo del medidor (Gris muy oscuro)
        g.setColour(juce::Colour(15, 15, 15));
        g.fillRoundedRectangle(bounds, 1.0f);

        // 2. Dibujo de la luz si hay audio
        if (level > 0.0f) {
            float yPos = bounds.getHeight() * (1.0f - level);
            auto fillArea = bounds.withTop(yPos);

            // Gradiente pro: Verde abajo, Amarillo al medio, Rojo arriba
            juce::ColourGradient gradient(juce::Colours::red, 0.0f, 0.0f,
                                          juce::Colours::limegreen, 0.0f, bounds.getHeight(), false);
            gradient.addColour(0.2, juce::Colours::orange);
            gradient.addColour(0.5, juce::Colours::yellow);

            g.setGradientFill(gradient);
            g.fillRoundedRectangle(fillArea, 1.0f);
        }

        // 3. (Opcional) Rayitas horizontales tipo cristal
        g.setColour(juce::Colour(30, 30, 30));
        for (int i = 1; i < 10; ++i) {
            float y = bounds.getHeight() * (i / 10.0f);
            g.drawHorizontalLine((int)y, 0, bounds.getWidth());
        }
    }

    void timerCallback() override {
        // La física del vúmetro: La luz cae suavemente si no hay audio nuevo
        if (level > 0.0f) {
            level -= 0.04f; // Velocidad de la caída (falloff)
            if (level < 0.0f) level = 0.0f;
            repaint();
        }
    }

private:
    float level = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PeakMeter)
};