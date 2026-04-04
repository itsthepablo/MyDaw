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

        float floorLUFS = -35.0f;
        float ceilingLUFS = history.referenceLUFS + 3.0f; 

        // --- 1. DIBUJAR LÍNEAS DE REFERENCIA ---
        float refY = lufsToY(history.referenceLUFS, area, floorLUFS, ceilingLUFS);
        
        // Línea de Objetivo (Roja)
        g.setColour(juce::Colours::red.withAlpha(0.8f));
        g.drawHorizontalLine((int)refY, area.getX(), area.getRight());
        
        g.setFont(10.0f);
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.drawText(juce::String((int)history.referenceLUFS) + " LUFS", 
                   area.getX() + 5, refY - 12, 60, 12, juce::Justification::left);


        // --- 2. DIBUJAR LA CURVA (SHORT-TERM) ---
        if (history.points.empty()) return;

        juce::Path strokeCurve;
        juce::Path fillCurve;
        
        bool firstInSegment = true;
        double lastSamplePos = -100.0;
        float segmentStartX = 0;
        
        // Umbral de salto: ahora que tenemos precisión de muestra, un salto pequeño
        // es suficiente para detectar un hueco técnico. 
        const double gapThreshold = 1000.0; // aprox 22ms a 44.1k

        auto finalizeSegment = [&](juce::Path& fill, float lastX) {
            if (!fill.isEmpty()) {
                fill.lineTo(lastX, area.getBottom());
                fill.lineTo(segmentStartX, area.getBottom());
                fill.closeSubPath();
            }
        };

        float lastX = 0;
        for (const auto& pair : history.points) {
            double sPos = pair.first;
            float lufsVal = pair.second;
            
            float x = (float)(sPos * hZoom) - hScroll;
            float y = lufsToY(lufsVal, area, floorLUFS, ceilingLUFS);
            
            // Culling horizontal básico
            if (x < -100.0f) { lastSamplePos = sPos; lastX = x; continue; }
            if (x > area.getRight() + 100.0f) break;

            // Detectar salto (Adelante o Atrás)
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
            // 2. DIBUJAR RELLENO (Look Profesional Tipo Youlean)
            g.setOpacity(0.4f);
            juce::ColourGradient grad(juce::Colours::orange.withAlpha(0.7f), 0.0f, area.getY(),
                                       juce::Colours::transparentWhite, 0.0f, area.getBottom(), false);
            g.setGradientFill(grad);
            g.fillPath(fillCurve);

            // 1. DIBUJAR TRAZO (Máxima Resolución)
            g.setOpacity(1.0f);
            g.setColour(juce::Colours::orange);
            g.strokePath(strokeCurve, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); 
        }
    }

private:
    static float lufsToY(float lufs, juce::Rectangle<float> area, float floor, float ceiling) {
        float normalized = (lufs - floor) / (ceiling - floor);
        normalized = juce::jlimit(0.0f, 1.0f, normalized);
        return area.getBottom() - (normalized * area.getHeight());
    }
};
