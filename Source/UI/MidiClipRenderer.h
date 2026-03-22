#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include <algorithm>
#include <cmath>

class MidiClipRenderer {
public:
    // --- FUNCIÓN DE DIBUJO ---
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
        
        // Estilo visual de "Edición Inline" (Grilla interior)
        if (isInlineEditingActive) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.fillRoundedRectangle(clipRect.toFloat(), 4.0f);
            
            g.setColour(juce::Colours::white.withAlpha(0.05f));
            for (float gridX = clipData.startX; gridX < clipData.startX + clipData.width; gridX += 20.0f) {
                int sx = clipRect.getX() + (int)((gridX - clipData.startX) * hZoom);
                g.drawVerticalLine(sx, (float)clipRect.getY(), (float)clipRect.getBottom());
            }
        }

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
        } else {
            // Diseño de patrón vacío
            g.setColour(baseColor.withAlpha(0.3f));
            for(int j = 6; j < clipRect.getHeight(); j+= 10) {
                g.drawHorizontalLine(clipRect.getY() + j, (float)clipRect.getX() + 2.0f, (float)clipRect.getRight() - 2.0f);
            }
        }
        g.restoreState();
    }

    // --- FUNCIÓN DE HIT-TESTING (Detección de clics en las notas) ---
    // Retorna el índice de la nota tocada, o -1 si no tocó ninguna
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
        
        // Iteramos en reversa para detectar primero las notas dibujadas por encima
        for (int i = (int)clipData.notes.size() - 1; i >= 0; --i) {
            const auto& note = clipData.notes[i];
            float normalizedY = 1.0f - ((float)(note.pitch - minPitch) / (float)pitchRange);
            float noteY = clipRect.getY() + 4.0f + (normalizedY * (clipRect.getHeight() - 12.0f));
            
            int noteScreenX = (int)(note.x * hZoom) - (int)hS;
            int noteScreenW = std::max(3, (int)(note.width * hZoom));
            
            juce::Rectangle<float> noteRect((float)noteScreenX, noteY, (float)noteScreenW, 5.0f);
            
            // Expandimos el área de clic para que sea más fácil agarrar notas diminutas
            if (noteRect.expanded(2.0f, 4.0f).contains((float)mouseX, (float)mouseY)) {
                return i;
            }
        }
        return -1;
    }
};