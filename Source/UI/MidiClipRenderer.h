#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include <algorithm>
#include <cmath>
#include <vector>

class MidiClipRenderer {
public:
    static void drawMidiClip(juce::Graphics& g,
        const MidiClipData& clipData,
        juce::Rectangle<int> clipRect,
        juce::Colour baseColor,
        bool isInlineEditingActive,
        float hZoom,
        float hS)
    {
        g.saveState();
        g.reduceClipRegion(clipRect);

        // --- 1. FONDO DE EDICIÓN INLINE ---
        if (isInlineEditingActive) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillRoundedRectangle(clipRect.toFloat(), 4.0f);

            g.setColour(juce::Colours::white.withAlpha(0.05f));
            for (float gridX = clipData.startX; gridX < clipData.startX + clipData.width; gridX += 20.0f) {
                int sx = clipRect.getX() + (int)((gridX - clipData.startX) * hZoom);
                g.drawVerticalLine(sx, (float)clipRect.getY(), (float)clipRect.getBottom());
            }
        }

        // --- 2. RENDERIZADO VISUAL "ESTRICTO" MULTICAPA DE AUTOMATIZACIÓN ---
        auto isLaneActive = [](const AutoLane& lane) {
            if (lane.nodes.size() > 1) return true;
            if (lane.nodes.size() == 1 && std::abs(lane.nodes[0].value - lane.defaultValue) > 0.001f) return true;
            return false;
            };

        // Estructura temporal para guardar los carriles activos y sus colores correspondientes
        struct ActiveLane {
            const AutoLane* lane;
            juce::Colour color;
        };
        std::vector<ActiveLane> lanesToDraw;

        // Evaluamos TODOS los carriles de forma independiente (Sin 'else if')
        if (isLaneActive(clipData.autoVol))    lanesToDraw.push_back({ &clipData.autoVol, juce::Colours::limegreen });
        if (isLaneActive(clipData.autoPan))    lanesToDraw.push_back({ &clipData.autoPan, juce::Colours::cyan });
        if (isLaneActive(clipData.autoFilter)) lanesToDraw.push_back({ &clipData.autoFilter, juce::Colours::orange });
        if (isLaneActive(clipData.autoPitch))  lanesToDraw.push_back({ &clipData.autoPitch, juce::Colours::magenta });

        // Dibujamos cada carril activo superponiéndolos
        for (const auto& activeItem : lanesToDraw) {
            if (activeItem.lane->nodes.empty()) continue;

            juce::Path autoPath;
            float startScreenX = (float)clipRect.getX();
            float bottomY = (float)clipRect.getBottom();
            float height = (float)clipRect.getHeight();

            const auto& nodes = activeItem.lane->nodes;

            autoPath.startNewSubPath(startScreenX, bottomY);

            for (const auto& node : nodes) {
                float clippedNodeX = juce::jlimit(0.0f, clipData.width, node.x);
                float screenX = startScreenX + (clippedNodeX * hZoom);
                float screenY = bottomY - (node.value * height);

                autoPath.lineTo(screenX, screenY);
            }

            float valAtEnd = activeItem.lane->getValueAt(clipData.width);
            autoPath.lineTo(startScreenX + (clipData.width * hZoom), bottomY - (valAtEnd * height));

            autoPath.lineTo(startScreenX + (clipData.width * hZoom), bottomY);
            autoPath.closeSubPath();

            // Rellenamos el fondo con el color específico de la automatización (baja opacidad para permitir superposición)
            g.setColour(activeItem.color.withAlpha(0.12f));
            g.fillPath(autoPath);

            // Dibujamos la línea de contorno más visible con su color respectivo
            g.setColour(activeItem.color.withAlpha(0.5f));
            g.strokePath(autoPath, juce::PathStrokeType(1.2f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
        }

        // --- 3. DIBUJO DE NOTAS MIDI (Capas superiores) ---
        if (!clipData.notes.empty()) {
            int minPitch = 127; int maxPitch = 0;
            for (const auto& n : clipData.notes) {
                if (n.pitch < minPitch) minPitch = n.pitch;
                if (n.pitch > maxPitch) maxPitch = n.pitch;
            }

            minPitch = std::max(0, minPitch - 3);
            maxPitch = std::min(127, maxPitch + 3);
            int pitchRange = std::max(12, maxPitch - minPitch);

            for (const auto& note : clipData.notes) {
                float normalizedY = 1.0f - ((float)(note.pitch - minPitch) / (float)pitchRange);
                float noteY = clipRect.getY() + 4.0f + (normalizedY * (clipRect.getHeight() - 12.0f));

                int noteScreenX = (int)(note.x * hZoom) - (int)hS;
                int noteScreenW = std::max(3, (int)(note.width * hZoom));

                juce::Rectangle<float> miniNoteRect((float)noteScreenX, noteY, (float)noteScreenW, 5.0f);

                g.setColour(baseColor.brighter(0.5f));
                g.fillRoundedRectangle(miniNoteRect, 1.0f);

                g.setColour(juce::Colours::black.withAlpha(0.8f));
                g.drawRoundedRectangle(miniNoteRect, 1.0f, 1.0f);
            }
        }
        else {
            g.setColour(baseColor.withAlpha(0.3f));
            for (int j = 6; j < clipRect.getHeight(); j += 10) {
                g.drawHorizontalLine(clipRect.getY() + j, (float)clipRect.getX() + 2.0f, (float)clipRect.getRight() - 2.0f);
            }
        }
        g.restoreState();
    }

    static int hitTestInlineNote(const MidiClipData& clipData,
        juce::Rectangle<int> clipRect,
        int mouseX, int mouseY,
        float hZoom, float hS)
    {
        if (clipData.notes.empty()) return -1;

        int minPitch = 127, maxPitch = 0;
        for (const auto& n : clipData.notes) {
            if (n.pitch < minPitch) minPitch = n.pitch;
            if (n.pitch > maxPitch) maxPitch = n.pitch;
        }
        minPitch = std::max(0, minPitch - 3);
        maxPitch = std::min(127, maxPitch + 3);
        int pitchRange = std::max(12, maxPitch - minPitch);

        for (int i = (int)clipData.notes.size() - 1; i >= 0; --i) {
            const auto& note = clipData.notes[i];
            float normalizedY = 1.0f - ((float)(note.pitch - minPitch) / (float)pitchRange);
            float noteY = clipRect.getY() + 4.0f + (normalizedY * (clipRect.getHeight() - 12.0f));

            int noteScreenX = (int)(note.x * hZoom) - (int)hS;
            int noteScreenW = std::max(3, (int)(note.width * hZoom));

            juce::Rectangle<float> noteRect((float)noteScreenX, noteY, (float)noteScreenW, 5.0f);

            if (noteRect.expanded(2.0f, 4.0f).contains((float)mouseX, (float)mouseY)) {
                return i;
            }
        }
        return -1;
    }
};