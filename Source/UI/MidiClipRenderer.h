#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include "MidiPatternStyles.h"
#include <algorithm>
#include <cmath>
#include <vector>

class MidiClipRenderer {
public:
    static void drawMidiClip(juce::Graphics& g,
        const MidiClipData& clipData,
        juce::Rectangle<int> clipRect,
        juce::Colour trackColor,
        juce::String name,
        bool isInlineEditingActive,
        float hZoom,
        float hS,
        double playheadPos = -1.0)
    {
        g.saveState();
        
        MidiStyleRecipe recipe = MidiStyleRegistry::getRecipe(clipData.style, trackColor);

        // --- 1. FONDO DEL CLIP (Sólido o Degradado) ---
        if (recipe.drawBackground) {
            if (recipe.useBackgroundGradient) {
                juce::ColourGradient cg(trackColor.darker(0.6f), (float)clipRect.getX(), (float)clipRect.getY(),
                                        juce::Colour(10, 10, 10), (float)clipRect.getX(), (float)clipRect.getBottom(), false);
                g.setGradientFill(cg);
            } else {
                g.setColour(recipe.backgroundColor);
            }
            g.fillRoundedRectangle(clipRect.toFloat(), 5.0f);
        }

        // --- 2. CABECERA (Header) y NOMBRE ---
        juce::Rectangle<float> headerRect = clipRect.withHeight(14.0f).toFloat();
        if (recipe.drawHeaderBackground) {
            g.setColour(recipe.backgroundColor.withAlpha(0.4f)); 
            g.fillRoundedRectangle(headerRect, 5.0f);
            
            g.setColour(juce::Colours::black.withAlpha(0.15f));
            g.drawHorizontalLine((int)headerRect.getBottom(), (float)clipRect.getX(), (float)clipRect.getRight());
        }

        // Texto del nombre (Premium: sincronizado con el color de nota del estilo)
        g.setColour(recipe.noteColor);
        g.setFont(juce::Font(11.0f, juce::Font::bold));
        g.drawText(" " + name, headerRect.reduced(3.0f, 0.0f).toNearestInt(),
                   juce::Justification::centredLeft, true);

        // --- 3. ÁREA INTERNA (Notas y Automatización) ---
        g.reduceClipRegion(clipRect); // Culling

        if (isInlineEditingActive) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillRoundedRectangle(clipRect.toFloat(), 4.0f);

            g.setColour(juce::Colours::white.withAlpha(0.05f));
            for (float gridX = clipData.startX; gridX < clipData.startX + clipData.width; gridX += 20.0f) {
                int sx = clipRect.getX() + (int)((gridX - clipData.startX) * hZoom);
                g.drawVerticalLine(sx, (float)clipRect.getY(), (float)clipRect.getBottom());
            }
        }

        // --- 2. RENDERIZADO VISUAL MULTICAPA DE AUTOMATIZACIÓN ---
        auto isLaneActive = [](const AutoLane& lane) {
            if (lane.nodes.size() > 1) return true;
            if (lane.nodes.size() == 1 && std::abs(lane.nodes[0].value - lane.defaultValue) > 0.001f) return true;
            return false;
            };

        struct ActiveLane {
            const AutoLane* lane;
            juce::Colour color;
        };
        std::vector<ActiveLane> lanesToDraw;

        if (isLaneActive(clipData.autoVol))    lanesToDraw.push_back({ &clipData.autoVol, juce::Colours::limegreen });
        if (isLaneActive(clipData.autoPan))    lanesToDraw.push_back({ &clipData.autoPan, juce::Colours::cyan });
        if (isLaneActive(clipData.autoFilter)) lanesToDraw.push_back({ &clipData.autoFilter, juce::Colours::orange });
        if (isLaneActive(clipData.autoPitch))  lanesToDraw.push_back({ &clipData.autoPitch, juce::Colours::magenta });

        for (const auto& activeItem : lanesToDraw) {
            if (activeItem.lane->nodes.empty()) continue;

            juce::Path autoPath;
            float startScreenX = (float)clipRect.getX();
            float bottomY = (float)clipRect.getBottom();
            float height = (float)clipRect.getHeight();

            const auto& nodes = activeItem.lane->nodes;

            autoPath.startNewSubPath(startScreenX, bottomY);

            // NODO INICIAL (Evaluamos el valor matemático exacto en el borde izquierdo del clip)
            float valAtStart = activeItem.lane->getValueAt(clipData.startX);
            autoPath.lineTo(startScreenX, bottomY - (valAtStart * height));

            for (const auto& node : nodes) {
                // --- EL BUG ESTABA AQUÍ: COORDENADAS RELATIVAS AL CLIP ---
                float localNodeX = node.x - clipData.startX;

                if (localNodeX < 0.0f) continue; // Si el nodo está ANTES del clip, no lo dibujamos
                if (localNodeX > clipData.width) break; // Si el nodo está DESPUÉS del clip, cortamos la ejecución para ahorrar CPU

                float screenX = startScreenX + (localNodeX * hZoom);
                float screenY = bottomY - (node.value * height);

                autoPath.lineTo(screenX, screenY);
            }

            // NODO FINAL (Evaluamos el valor matemático exacto en el borde derecho del clip)
            float valAtEnd = activeItem.lane->getValueAt(clipData.startX + clipData.width);
            autoPath.lineTo(startScreenX + (clipData.width * hZoom), bottomY - (valAtEnd * height));

            autoPath.lineTo(startScreenX + (clipData.width * hZoom), bottomY);
            autoPath.closeSubPath();

            g.setColour(activeItem.color.withAlpha(0.12f));
            g.fillPath(autoPath);

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

                // --- DIBUJO RELATIVO: Sumamos clipData.startX para posicionar la nota 0-based en el mundo real ---
                float worldX = clipData.startX + (note.x - clipData.offsetX);
                int noteScreenX = (int)(worldX * hZoom) - (int)hS;
                int noteScreenW = std::max(3, (int)(note.width * hZoom));

                juce::Rectangle<float> miniNoteRect((float)noteScreenX, noteY, (float)noteScreenW, 5.0f);

                // --- NUEVO: EFECTO ACTIVE GLOW (Al pasar el Cabezal) ---
                bool isActive = (playheadPos >= worldX && playheadPos <= (worldX + note.width));
                juce::Colour finalNoteColor = isActive ? recipe.activeNoteColor : recipe.noteColor;
                
                if (isActive && recipe.activeUseGradient) {
                    // Gradiente Especial para Classic (Reflejo Blanco sobre Negro)
                    juce::Colour highlightColor = juce::Colours::white.withAlpha(0.9f);
                    juce::ColourGradient cg(highlightColor, miniNoteRect.getX(), miniNoteRect.getY(),
                                            recipe.activeNoteColor, miniNoteRect.getX(), miniNoteRect.getBottom(), false);
                    g.setGradientFill(cg);
                }
                else if (recipe.useGradient || (isActive && !recipe.activeUseGradient)) {
                    // Gradiente de Neón para los otros estilos (Glow)
                    juce::Colour topColor = isActive ? recipe.activeNoteColor : recipe.noteColor.brighter(0.4f);
                    juce::ColourGradient cg(topColor, miniNoteRect.getX(), miniNoteRect.getY(),
                                            recipe.activeNoteColor.darker(0.1f), miniNoteRect.getX(), miniNoteRect.getBottom(), false);
                    g.setGradientFill(cg);
                } else {
                    g.setColour(finalNoteColor);
                }
                
                g.fillRoundedRectangle(miniNoteRect, recipe.noteCornerRadius);

                g.setColour(isActive ? finalNoteColor.brighter(0.2f) : recipe.noteBorder);
                g.drawRoundedRectangle(miniNoteRect, recipe.noteCornerRadius, 1.0f);
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

            float worldX = clipData.startX + (note.x - clipData.offsetX);
            int noteScreenX = (int)(worldX * hZoom) - (int)hS;
            int noteScreenW = std::max(3, (int)(note.width * hZoom));

            juce::Rectangle<float> noteRect((float)noteScreenX, noteY, (float)noteScreenW, 5.0f);

            if (noteRect.expanded(2.0f, 4.0f).contains((float)mouseX, (float)mouseY)) {
                return i;
            }
        }
        return -1;
    }

    static void drawMidiSummary(juce::Graphics& g,
        const MidiClipData& clipData,
        juce::Rectangle<int> area,
        float hZoom,
        float hS,
        int minP,
        int maxP)
    {
        if (clipData.notes.empty()) return;

        const int height = area.getHeight();
        if (height <= 0) return;

        // Utilizamos el rango global pasado como parámetro para alineación vertical correcta
        int pitchRange = std::max(1, maxP - minP);

        // COLOR BLANCO CON 80% DE OPACIDAD
        g.setColour(juce::Colours::white.withAlpha(0.8f));

        for (const auto& note : clipData.notes) {
            float normalizedY = 1.0f - ((float)(note.pitch - minP) / (float)pitchRange);
            float noteY = (float)area.getY() + (normalizedY * (height - 4.0f)) + 2.0f;

            float worldX = clipData.startX + (note.x - clipData.offsetX);
            int noteScreenX = (int)(worldX * hZoom) - (int)hS;
            int noteScreenW = std::max(2, (int)(note.width * hZoom));

            // Solo renderizar si es visible horizontalmente
            if (noteScreenX + noteScreenW > 0 && noteScreenX < (area.getX() + area.getWidth())) {
                g.fillRect((float)noteScreenX, noteY, (float)noteScreenW, 3.0f);
            }
        }
    }
};