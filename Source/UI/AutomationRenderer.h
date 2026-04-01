#pragma once
#include <JuceHeader.h>
#include "../Data/AutomationData.h"

class AutomationRenderer {
public:
    static void drawAutomationLane(juce::Graphics& g, const juce::Rectangle<int>& laneRect, const AutomationClipData* aut, float hZoom, float hS) {
        if (!aut || !aut->isShowing) return;

        // Overlay translúcido sobre el track
        g.setColour(juce::Colour(10, 10, 15).withAlpha(0.4f));
        g.fillRect(laneRect);

        g.setColour(aut->color.brighter());

        // Si no hay nodos insertados, pintamos la línea base (defaultValue)
        if (aut->lane.nodes.empty()) {
            float valY = laneRect.getBottom() - (aut->lane.defaultValue * laneRect.getHeight());
            g.drawHorizontalLine((int)valY, 0.0f, (float)laneRect.getWidth());
        } else {
            juce::Path p;
            bool first = true;
            // Simplificación: iteramos por píxeles o intervalos cortos
            for (float tX = 0; tX <= laneRect.getWidth(); tX += 3.0f) {
                float realTime = (tX + hS) / hZoom;
                float val = aut->lane.getValueAt(realTime);
                float valY = laneRect.getBottom() - (val * laneRect.getHeight());
                if (first) { p.startNewSubPath(tX, valY); first = false; }
                else p.lineTo(tX, valY);
            }
            g.strokePath(p, juce::PathStrokeType(2.5f));

            // Puntos (Nodos)
            for (const auto& node : aut->lane.nodes) {
                float screenX = (node.x * hZoom) - hS;
                if (screenX >= 0 && screenX <= laneRect.getWidth()) {
                    float valY = laneRect.getBottom() - (node.value * laneRect.getHeight());
                    g.setColour(juce::Colours::white);
                    g.fillEllipse(screenX - 4.0f, valY - 4.0f, 8.0f, 8.0f);
                    g.setColour(aut->color.brighter());
                    g.drawEllipse(screenX - 4.0f, valY - 4.0f, 8.0f, 8.0f, 1.5f);
                }
            }
        }

        // Etiqueta indicadora
        juce::Rectangle<int> lblRect = laneRect.withHeight(16).withY(laneRect.getY() + 2);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillRect(lblRect);
        g.setColour(aut->color.brighter());
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText(" " + aut->name, lblRect, juce::Justification::centredLeft, true);
    }
};
