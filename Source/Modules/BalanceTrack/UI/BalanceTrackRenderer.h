#pragma once
#include <JuceHeader.h>
#include "../Bridges/BalanceTrackBridge.h"

/**
 * BalanceTrackRenderer — Renderizador especializado para el modulo BalanceTrack.
 */
class BalanceTrackRenderer
{
public:
    static void drawBalanceTrack(juce::Graphics& g, 
                                 BalanceTrackBridge* bridge,
                                 juce::Rectangle<int> area,
                                 double hZoom, 
                                 double hScroll)
    {
        if (!bridge || !bridge->isActive()) return;
        auto& points = bridge->getPoints();

        g.setColour(juce::Colours::white.withAlpha(0.2f));
        float centerY = (float)area.getCentreY();
        g.drawHorizontalLine((int)centerY, (float)area.getX(), (float)area.getRight());
        
        g.setFont(10.0f);
        g.drawText("CENTER", area.getX() + 5, (int)centerY - 12, 50, 10, juce::Justification::left);

        if (points.empty()) return;

        juce::Path strokeCurve;
        juce::Path fillCurveL; 
        juce::Path fillCurveR; 
        
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
        float maxDB = bridge->getReferenceScale();

        for (const auto& pair : points) {
            double sPos = pair.first;
            float dbVal = pair.second;
            
            float x = (float)(sPos * hZoom) - (float)hScroll;
            float normVal = juce::jlimit(-1.0f, 1.0f, dbVal / maxDB);
            float y = centerY - (normVal * (area.getHeight() / 2.0f) * 0.8f);
            
            float yL = (dbVal > 0.0f) ? y : centerY;
            float yR = (dbVal < 0.0f) ? y : centerY;

            if (x < -100.0f) { lastSamplePos = sPos; lastX = x; continue; }
            if (x > (float)area.getRight() + 100.0f) break;

            bool isJump = (lastSamplePos >= 0) && (sPos - lastSamplePos) > gapThreshold;

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
            g.setOpacity(1.0f);
            g.setColour(juce::Colours::cyan.withAlpha(0.35f));
            g.fillPath(fillCurveL);
            g.setColour(juce::Colours::magenta.withAlpha(0.35f));
            g.fillPath(fillCurveR);
            g.setColour(juce::Colours::white.withAlpha(0.85f));
            g.strokePath(strokeCurve, juce::PathStrokeType(1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded)); 
        }
    }
};
