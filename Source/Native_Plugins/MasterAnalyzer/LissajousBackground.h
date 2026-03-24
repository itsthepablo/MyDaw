/*
  ==============================================================================
    LissajousBackground.h
    Fondo Vectorscopio - SUTIL (40% Opacidad).
    - AJUSTE: Opacidad bajada a 0.4f en líneas, texto y punto central.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class LissajousBackground
{
public:
    static void draw(juce::Graphics& g, float centerX, float centerY, float scaleX, float scaleY)
    {
        // 1. DIBUJAR LA ESTRUCTURA (GRID)
        juce::Path gridPath;
        for (int i = 0; i < 8; ++i)
        {
            float angle = i * juce::MathConstants<float>::pi * 0.25f;
            gridPath.startNewSubPath(centerX, centerY);
            float endX = centerX + std::cos(angle) * scaleX;
            float endY = centerY - std::sin(angle) * scaleY;
            gridPath.lineTo(endX, endY);
        }

        // 2. APLICAR ESTILO DE PUNTOS
        juce::Path dottedPath;
        float dashPattern[] = { 0.5f, 6.0f };

        juce::PathStrokeType strokeType(2.0f);
        strokeType.createDashedStroke(dottedPath, gridPath, dashPattern, 2);
        strokeType.setEndStyle(juce::PathStrokeType::rounded);

        // CAMBIO: Opacidad bajada a 0.4f
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.fillPath(dottedPath);


        // 3. ETIQUETAS
        // CAMBIO: Opacidad bajada a 0.4f
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.setFont(12.0f);

        float labelDist = 20.0f;

        auto drawLabel = [&](juce::String text, float angleDeg) {
            float angleRad = angleDeg * (juce::MathConstants<float>::pi / 180.0f);
            float tipX = centerX + std::cos(angleRad) * scaleX;
            float tipY = centerY - std::sin(angleRad) * scaleY;
            float lblX = tipX + std::cos(angleRad) * labelDist;
            float lblY = tipY - std::sin(angleRad) * labelDist;
            juce::Rectangle<int> r((int)lblX - 15, (int)lblY - 10, 30, 20);
            g.drawText(text, r, juce::Justification::centred, false);
            };

        drawLabel("+R", 45.0f);
        drawLabel("+L", 135.0f);
        drawLabel("-R", 225.0f);
        drawLabel("-L", 315.0f);

        // 4. PUNTO CENTRAL
        // CAMBIO: Opacidad bajada a 0.4f
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.fillRect(centerX - 1.5f, centerY - 1.5f, 3.0f, 3.0f);
    }
};