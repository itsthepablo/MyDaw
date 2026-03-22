#pragma once
#include <JuceHeader.h>
#include "PlaylistTool.h"
#include "../PlaylistComponent.h"

class ScissorTool : public PlaylistTool {
public:
    void mouseDown(const juce::MouseEvent& e, PlaylistComponent& p) override {
        int cIdx = p.getClipAt(e.x, e.y);
        if (cIdx == -1) return;

        float hS = (float)p.hBar.getCurrentRangeStart();
        float absX = (e.x + hS) / p.hZoom;

        // 1. Calculamos el punto de corte respetando el Grid (Snap)
        float splitX = std::round(absX / p.snapPixels) * p.snapPixels;

        auto& clip = p.clips[cIdx];

        // Evitamos que corten justo en el borde inicio o final (crearía clips de ancho cero)
        if (splitX <= clip.startX || splitX >= clip.startX + clip.width) return;

        float newWidthLeft = splitX - clip.startX;
        float newWidthRight = clip.width - newWidthLeft;

        // Bloqueamos el audio por seguridad mientras manipulamos la memoria
        std::unique_ptr<juce::ScopedLock> lock;
        if (p.audioMutex) lock = std::make_unique<juce::ScopedLock>(*p.audioMutex);

        // --- CORTE DE CLIP MIDI ---
        if (clip.linkedMidi) {
            auto* oldMidi = clip.linkedMidi;

            // Creamos el clip de la mitad derecha
            auto* newMidi = new MidiClipData();
            newMidi->name = oldMidi->name + " (Corte)";
            newMidi->startX = splitX;
            newMidi->width = newWidthRight;
            newMidi->color = oldMidi->color;

            // Repartimos las notas: Las que están después del corte se mudan al nuevo clip
            for (auto it = oldMidi->notes.begin(); it != oldMidi->notes.end(); ) {
                if (it->x >= splitX) {
                    newMidi->notes.push_back(*it);
                    it = oldMidi->notes.erase(it); // Se borra del clip izquierdo
                }
                else {
                    ++it;
                }
            }

            // Acortamos el clip izquierdo original
            oldMidi->width = newWidthLeft;
            clip.width = newWidthLeft;

            // Añadimos el nuevo clip al motor y a la vista
            clip.trackPtr->midiClips.add(newMidi);
            p.clips.push_back({ clip.trackPtr, newMidi->startX, newMidi->width, newMidi->name, nullptr, newMidi });
        }

        // --- CORTE DE AUDIO (VISUAL POR AHORA) ---
        else if (clip.linkedAudio) {
            // Nota: Un corte "Real" de audio requiere añadir un offset de lectura en TrackProcessor.
            // Por ahora hacemos un corte visual (Trunca el final)
            auto* oldAudio = clip.linkedAudio;
            oldAudio->width = newWidthLeft;
            clip.width = newWidthLeft;
        }

        p.repaint();
    }

    void mouseDrag(const juce::MouseEvent& e, PlaylistComponent& p) override {}
    void mouseUp(const juce::MouseEvent& e, PlaylistComponent& p) override {}

    void mouseMove(const juce::MouseEvent& e, PlaylistComponent& p) override {
        // Hacemos que el puntero se vea como una cruz para que parezca una herramienta de corte
        p.setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }
};