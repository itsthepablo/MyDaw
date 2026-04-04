#pragma once
#include <JuceHeader.h>
#include "../../Tracks/Track.h"

// ==============================================================================
// BalanceRenderer — Renderizador especializado para la pista de Balance estéreo.
// Dibuja una curva de desviación (L/R) alrededor de un eje central.
// ==============================================================================
class BalanceRenderer
{
public:
    static void drawBalanceTrack(juce::Graphics& g, 
                                 const BalanceHistory& history,
                                 juce::Rectangle<int> area,
                                 double hZoom, 
                                 double hScroll)
    {
        // 1. DIBUJAR LÍNEA DE REFERENCIA (CENTRO 0dB)
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        float centerY = (float)area.getCentreY();
        g.drawHorizontalLine((int)centerY, (float)area.getX(), (float)area.getRight());
        
        // Texto de referencia Centrado
        g.setFont(10.0f);
        g.drawText("CENTER", area.getX() + 5, (int)centerY - 12, 50, 10, juce::Justification::left);

        if (history.points.empty()) return;

        juce::Path strokeCurve;
        juce::Path fillCurveL; // Hacia arriba (Cyan)
        juce::Path fillCurveR; // Hacia abajo (Magenta)
        
        bool firstInSegment = true;
        double lastSamplePos = -100.0;
        float segmentStartX = 0;
        const double gapThreshold = 2000.0; 

        auto finalizeSegment = [&](juce::Path& fillL, juce::Path& fillR, float lastX) {
            if (!fillL.isEmpty()) {
                fillL.lineTo(lastX, centerY);
                fillL.lineTo(segmentStartX, centerY);
                fillL.closeSubPath();
            }
            if (!fillR.isEmpty()) {
                fillR.lineTo(lastX, centerY);
                fillR.lineTo(segmentStartX, centerY);
                fillR.closeSubPath();
            }
        };

        float lastX = 0;
        float maxDB = history.referenceScaleDB;

        for (const auto& pair : history.points) {
            double sPos = pair.first;
            float dbVal = pair.second; // Positivo = L, Negativo = R
            
            float x = (float)(sPos * hZoom) - hScroll;
            
            // Mapeo vertical: 0dB -> CenterY, +MaxDB -> Top, -MaxDB -> Bottom
            float normVal = juce::jlimit(-1.0f, 1.0f, dbVal / maxDB);
            float y = centerY - (normVal * (area.getHeight() / 2.0f) * 0.8f);
            
            // Separación de puntos para los rellenos:
            // fillCurveL solo "salta" hacia arriba (Cian)
            // fillCurveR solo "salta" hacia abajo (Magenta)
            float yL = (dbVal > 0.0f) ? y : centerY;
            float yR = (dbVal < 0.0f) ? y : centerY;

            if (x < -100.0f) { lastSamplePos = sPos; lastX = x; continue; }
            if (x > area.getRight() + 100.0f) break;

            bool isJump = (lastSamplePos >= 0) && (sPos < lastSamplePos || (sPos - lastSamplePos) > gapThreshold);

            if (firstInSegment || isJump) {
                if (isJump) finalizeSegment(fillCurveL, fillCurveR, lastX);
                
                strokeCurve.startNewSubPath(x, y);
                fillCurveL.startNewSubPath(x, centerY);
                fillCurveL.lineTo(x, yL);
                fillCurveR.startNewSubPath(x, centerY);
                fillCurveR.lineTo(x, yR);
                
                segmentStartX = x;
                firstInSegment = false;
            } else {
                strokeCurve.lineTo(x, y);
                fillCurveL.lineTo(x, yL);
                fillCurveR.lineTo(x, yR);
            }
            
            lastSamplePos = sPos;
            lastX = x;
        }
        finalizeSegment(fillCurveL, fillCurveR, lastX);

        if (!strokeCurve.isEmpty()) {
            // 2. DIBUJAR RELLENOS SÓLIDOS TRANSLÚCIDOS
            // Lado Izquierdo (Arriba - Cian Sólido)
            g.setOpacity(1.0f);
            g.setColour(juce::Colours::cyan.withAlpha(0.35f));
            g.fillPath(fillCurveL);

            // Lado Derecho (Abajo - Magenta Sólido)
            g.setColour(juce::Colours::magenta.withAlpha(0.35f));
            g.fillPath(fillCurveR);

            // 3. DIBUJAR TRAZO PRINCIPAL (Brillante)
            g.setColour(juce::Colours::white.withAlpha(0.85f));
            g.strokePath(strokeCurve, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); 
        }
    }
};
