#pragma once
#include <JuceHeader.h>
#include "../Bridges/LoudnessTrackBridge.h"

/**
 * LoudnessTrackRenderer — Lógica de dibujado de la curva de Loudness.
 * Estilo avanzado con degradados y precisión de muestra.
 */
class LoudnessTrackRenderer {
public:
    static void drawLoudnessTrack(juce::Graphics& g, juce::Rectangle<float> area, 
                                 LoudnessTrackBridge* bridge, float hZoom, float hScroll) 
    {
        if (!bridge || !bridge->isActive()) return;

        auto& points = bridge->getLoudnessPoints();
        if (points.empty()) return;

        float floorLUFS = -35.0f;
        float ceilingLUFS = bridge->getReferenceLUFS() + 3.0f; 

        // --- 1. LÍNEA DE REFERENCIA ---
        float refY = lufsToY(bridge->getReferenceLUFS(), area, floorLUFS, ceilingLUFS);
        g.setColour(juce::Colours::red.withAlpha(0.8f));
        g.drawHorizontalLine((int)refY, area.getX(), area.getRight());

        // --- 2. DIBUJAR CURVA ---
        juce::Path strokeCurve;
        juce::Path fillCurve;
        
        bool firstInSegment = true;
        double lastSamplePos = -100.0;
        float segmentStartX = 0;
        const double gapThreshold = 1000.0;

        auto finalizeSegment = [&](juce::Path& fill, float lastX) {
            if (!fill.isEmpty()) {
                fill.lineTo(lastX, area.getBottom());
                fill.lineTo(segmentStartX, area.getBottom());
                fill.closeSubPath();
            }
        };

        float lastX = 0;
        for (const auto& pair : points) {
            double sPos = pair.first;
            float lufsVal = pair.second;
            
            float x = (float)(sPos * hZoom) - hScroll;
            float y = lufsToY(lufsVal, area, floorLUFS, ceilingLUFS);
            
            if (x < -50.0f) { lastSamplePos = sPos; lastX = x; continue; }
            if (x > area.getRight() + 50.0f) break;

            bool isJump = (lastSamplePos >= 0) && (sPos < lastSamplePos || (sPos - lastSamplePos) > gapThreshold);

            if (firstInSegment || isJump) {
                if (isJump) finalizeSegment(fillCurve, lastX);
                strokeCurve.startNewSubPath(x, y);
                fillCurve.startNewSubPath(x, y);
                segmentStartX = x;
                firstInSegment = false;
            } else {
                strokeCurve.lineTo(x, y);
                fillCurve.lineTo(x, y);
            }
            
            lastSamplePos = sPos;
            lastX = x;
        }
        finalizeSegment(fillCurve, lastX);

        if (!strokeCurve.isEmpty()) {
            g.setOpacity(0.4f);
            juce::ColourGradient grad(juce::Colours::orange.withAlpha(0.6f), 0.0f, area.getY(),
                                       juce::Colours::transparentWhite, 0.0f, area.getBottom(), false);
            g.setGradientFill(grad);
            g.fillPath(fillCurve);

            g.setOpacity(1.0f);
            g.setColour(juce::Colours::orange);
            g.strokePath(strokeCurve, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); 
        }
    }

private:
    static float lufsToY(float lufs, juce::Rectangle<float> area, float floor, float ceiling) {
        float normalized = (lufs - floor) / (ceiling - floor);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);
        return area.getBottom() - (normalized * area.getHeight());
    }
};
