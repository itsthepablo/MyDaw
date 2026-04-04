#pragma once
#include <JuceHeader.h>
#include "../../Tracks/Track.h"

class LoudnessRenderer {
public:
    static void drawLoudnessTrack(juce::Graphics& g, juce::Rectangle<float> area, 
                                 Track* track, float hZoom, float hScroll) 
    {
        if (!track || track->getType() != TrackType::Loudness) return;

        auto& history = track->loudnessHistory;
        if (!history.isActive) return;

        // --- 1. DIBUJAR LÍNEAS DE REFERENCIA ---
        float refY = lufsToY(history.referenceLUFS, area);
        
        // Línea de -23 LUFS (Roja)
        g.setColour(juce::Colours::red.withAlpha(0.6f));
        g.drawHorizontalLine((int)refY, area.getX(), area.getRight());
        
        g.setFont(10.0f);
        g.drawText(juce::String((int)history.referenceLUFS) + " LUFS", 
                   area.getX() + 5, refY - 12, 60, 12, juce::Justification::left);

        // Guías de escala (-18, -40)
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        float y18 = lufsToY(-18.0f, area);
        float y40 = lufsToY(-40.0f, area);
        g.drawHorizontalLine((int)y18, area.getX(), area.getRight());
        g.drawHorizontalLine((int)y40, area.getX(), area.getRight());

        // --- 2. DIBUJAR LA CURVA (SHORT-TERM) ---
        if (history.points.empty()) return;

        juce::Path curve;
        bool first = true;

        for (const auto& p : history.points) {
            float x = (float)(p.samplePos * hZoom) - hScroll; // Mapeo directo 1:1 con arreglo
            
            // Culling horizontal
            if (x < -100.0f) continue;
            if (x > area.getRight() + 100.0f) break;

            float y = lufsToY(p.shortTermLUFS, area);

            if (first) {
                curve.startNewSubPath(x, y);
                first = false;
            } else {
                curve.lineTo(x, y);
            }
        }

        if (!curve.isEmpty()) {
            g.setOpacity(0.8f);
            g.setColour(juce::Colours::orange);
            g.strokePath(curve, juce::PathStrokeType(1.5f));
            
            // Opcional: Relleno suave bajo la curva
            juce::Path fill = curve;
            fill.lineTo(fill.getCurrentPosition().getX(), area.getBottom());
            
            // Encontrar el primer punto visible para cerrar el relleno
            float firstX = (float)(history.points.front().samplePos * hZoom) - hScroll;
            fill.lineTo(firstX, area.getBottom());
            fill.closeSubPath();
            
            juce::ColourGradient grad(juce::Colours::orange.withAlpha(0.2f), 0.0f, area.getY(),
                                       juce::Colours::transparentWhite, 0.0f, area.getBottom(), false);
            g.setGradientFill(grad);
            g.fillPath(fill);
            g.setOpacity(1.0f);
        }
    }

private:
    static float lufsToY(float lufs, juce::Rectangle<float> area) {
        // Escala: 0 LUFS arriba, -60 LUFS abajo (ajustable)
        float normalized = (lufs - (-60.0f)) / (0.0f - (-60.0f));
        normalized = juce::jlimit(0.0f, 1.0f, normalized);
        return area.getBottom() - (normalized * area.getHeight());
    }
};
