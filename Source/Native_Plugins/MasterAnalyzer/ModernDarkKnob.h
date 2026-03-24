/*
  ==============================================================================
    ModernDarkKnob.h
    Estilo "Dark Modern" - VERSIÓN REFINADA (SUTIL).
    - Líneas más finas.
    - Brillo (Glow) reducido y más compacto.
    - Mantiene el centro transparente para ver el texto LUFS.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <cmath> 

class ModernDarkKnob : public juce::LookAndFeel_V4
{
public:
    ModernDarkKnob() {}

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat();
        auto center = bounds.getCentre();
        float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;

        // Ajuste de radios (Mismas proporciones para no mover nada)
        float ledRingRadius = radius * 0.85f;
        float knobTrackRadius = radius * 0.65f;

        // --- DEFINICIÓN DE GROSORES SUTILES ---
        float strokeWidth = 2.0f;

        // Patrón de línea discontinua: { largo_linea, espacio_vacio }
        float dashPattern[] = { 2.5f, 5.0f };

        juce::PathStrokeType stroke(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        juce::PathStrokeType strokeGlow(strokeWidth + 3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        // --- 1. ANILLO DE FONDO (LEDs Apagados) ---
        juce::Path bgArc;
        bgArc.addCentredArc(center.x, center.y, ledRingRadius, ledRingRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);

        juce::Path bgDashed;
        stroke.createDashedStroke(bgDashed, bgArc, dashPattern, 2);

        // Gris muy oscuro y sutil para el fondo
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.strokePath(bgDashed, stroke);

        // --- 2. ANILLO ACTIVO (LEDs Encendidos) ---
        float currentAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        if (currentAngle > rotaryStartAngle)
        {
            juce::Path activeArc;
            activeArc.addCentredArc(center.x, center.y, ledRingRadius, ledRingRadius, 0.0f, rotaryStartAngle, currentAngle, true);

            juce::Path activeDashed;
            stroke.createDashedStroke(activeDashed, activeArc, dashPattern, 2);

            // Glow Cyan (Mucho más sutil ahora)
            g.setColour(juce::Colours::cyan.withAlpha(0.25f));
            g.strokePath(activeDashed, strokeGlow);

            // LED Blanco brillante (El núcleo fino)
            g.setColour(juce::Colours::white.withAlpha(0.95f));
            g.strokePath(activeDashed, stroke);
        }

        // --- 3. EL CUERPO DEL KNOB (Solo bordes finos) ---
        // Anillo oscuro semitransparente (Base)
        g.setColour(juce::Colour::fromRGB(40, 42, 45).withAlpha(0.3f));
        g.drawEllipse(center.x - knobTrackRadius, center.y - knobTrackRadius, knobTrackRadius * 2, knobTrackRadius * 2, 8.0f);

        // Borde metálico exterior (Más fino: 1.5f)
        g.setColour(juce::Colour::fromRGB(80, 82, 85));
        g.drawEllipse(center.x - knobTrackRadius, center.y - knobTrackRadius, knobTrackRadius * 2, knobTrackRadius * 2, 1.5f);

        // Borde interior sutil (para dar volumen)
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawEllipse(center.x - knobTrackRadius + 4, center.y - knobTrackRadius + 4, (knobTrackRadius - 4) * 2, (knobTrackRadius - 4) * 2, 1.0f);


        // --- 4. PUNTO INDICADOR (DOT) ---
        float dotRadius = 3.0f; // Un poco más pequeño
        float dotDist = knobTrackRadius;

        // Ajuste matemático de ángulo (-PI/2) para corregir la rotación visual
        float dotX = center.x + std::cos(currentAngle - juce::MathConstants<float>::halfPi) * dotDist;
        float dotY = center.y + std::sin(currentAngle - juce::MathConstants<float>::halfPi) * dotDist;

        // Glow del punto (reducido)
        g.setColour(juce::Colours::cyan.withAlpha(0.3f));
        g.fillEllipse(dotX - 5, dotY - 5, 10, 10);

        // Punto blanco sólido
        g.setColour(juce::Colours::white);
        g.fillEllipse(dotX - dotRadius, dotY - dotRadius, dotRadius * 2, dotRadius * 2);
    }
};