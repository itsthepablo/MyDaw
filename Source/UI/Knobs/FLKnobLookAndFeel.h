#pragma once
#include <JuceHeader.h>

// ==============================================================================
// Clase compartida para estilizar los knobs pequeños estilo FL Studio
// ==============================================================================
class FLKnobLookAndFeel : public juce::LookAndFeel_V4 {
public:
    FLKnobLookAndFeel() {
        // Colores base estilo FL
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(255, 180, 0)); // Relleno naranja brillante
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(40, 42, 45)); // Fondo muy oscuro
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportion, float rotaryStartAngle, float rotaryEndAngle,
                          juce::Slider& slider) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toX = bounds.getCentreX();
        auto toY = bounds.getCentreY();
        auto lineW = bounds.getWidth() > 30.0f ? 3.0f : 2.0f; // Ajuste dinámico de grosor
        auto arcRadius = radius - lineW * 0.5f;

        // Dibujar fondo del círculo sólido
        g.setColour(findColour(juce::Slider::rotarySliderOutlineColourId));
        g.fillEllipse(toX - radius, toY - radius, radius * 2.0f, radius * 2.0f);

        // Arco de fondo (guía sutil)
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(toX, toY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (slider.isEnabled()) {
            float centerAngle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * 0.5f;
            float currentAngle = rotaryStartAngle + sliderPosProportion * (rotaryEndAngle - rotaryStartAngle);
            
            juce::Path valueArc;
            // Si el rango empieza en negativo (paneo), dibujamos desde el centro
            if (slider.getMinimum() < 0.0) {
                valueArc.addCentredArc(toX, toY, arcRadius, arcRadius, 0.0f, centerAngle, currentAngle, true);
            } else {
                valueArc.addCentredArc(toX, toY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, currentAngle, true);
            }

            // Dibujar arco de progreso (Naranja)
            g.setColour(findColour(juce::Slider::rotarySliderFillColourId));
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            
            // Dibujar la línea indicadora central (Blanca)
            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.drawLine(toX, toY, 
                       toX + (radius * 0.75f) * std::cos(currentAngle - juce::MathConstants<float>::halfPi),
                       toY + (radius * 0.75f) * std::sin(currentAngle - juce::MathConstants<float>::halfPi), 
                       1.5f);
        }
    }
};