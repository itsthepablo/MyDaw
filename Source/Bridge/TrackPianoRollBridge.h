#pragma once
#include <JuceHeader.h>
#include "../Tracks/UI/TrackContainer.h"
#include "../Playlist/PlaylistComponent.h"
#include "../PianoRoll/PianoRollComponent.h"

class TrackPianoRollBridge {
public:
    static void connect(TrackContainer& container,
        PlaylistComponent& playlist,
        PianoRollComponent& ui,
        std::function<void()> onOpenPianoRoll)
    {
        // --- MOTOR GLOBAL DE SINCRONIZACIÓN DE CLONES (GHOST COPIES) ---
        auto syncEngine = [&container, &playlist](MidiPattern* sourceClip) {
            if (!sourceClip) return;

            // --- AUTO-CRECIMIENTO DINÁMICO (Siempre activo) ---
            double contentExt = 0.0;
            for (const auto& n : sourceClip->getNotes()) { if (n.x + n.width > contentExt) contentExt = n.x + n.width; }
            double snappedExt = (contentExt <= 0.0) ? 1280.0 : std::ceil(contentExt / 320.0) * 320.0;
            float finalWidth = (float)std::max(1280.0, snappedExt);
            
            sourceClip->setWidth(finalWidth);

            // --- ACTUALIZACIÓN DEL CACHÉ VISUAL DE LA PLAYLIST ---
            // Buscamos el bloque visual en la Playlist y actualizamos su ancho
            for (auto& clip : playlist.clips) {
                if (clip.linkedMidi == sourceClip || (clip.linkedMidi && clip.linkedMidi->getName() == sourceClip->getName())) {
                    clip.width = finalWidth;
                }
            }

            for (auto* track : container.getTracks()) {
                bool trackModified = false;
                for (auto* targetClip : track->getMidiClips()) {
                    if (targetClip == sourceClip) {
                        trackModified = true;
                    }
                    else if (targetClip->getName() == sourceClip->getName()) {
                        targetClip->getNotes() = sourceClip->getNotes();
                        targetClip->autoVol = sourceClip->autoVol;
                        targetClip->autoPan = sourceClip->autoPan;
                        targetClip->autoPitch = sourceClip->autoPitch;
                        targetClip->autoFilter = sourceClip->autoFilter;
                        targetClip->setColor(sourceClip->getColor());
                        targetClip->setWidth(finalWidth);
                        trackModified = true;
                    }
                }
                
                if (trackModified) {
                    track->commitSnapshot();
                }
            }
            
            playlist.updateScrollBars(); // Recalcular límites de scroll en Playlist
            playlist.repaint();
        };

        // Enlazamos el motor tanto al Piano Roll como a la edición Inline de la Playlist
        ui.onPatternEdited = syncEngine;
        playlist.onPatternEdited = syncEngine;


        container.onOpenPianoRoll = [&container, &playlist, &ui, onOpenPianoRoll](Track& t) {
            t.migrateMidiToRelative(); // MIGRACIÓN AUTOMÁTICA
            MidiPattern* targetClip = nullptr;

            if (t.getMidiClips().size() > 0) {
                targetClip = t.getMidiClips().getFirst();
            }
            else {
                int maxPatternNum = 0;
                for (auto* tr : container.getTracks()) {
                    for (auto* mc : tr->getMidiClips()) {
                        if (mc->getName().startsWith("Pattern ")) {
                            int num = mc->getName().substring(8).getIntValue();
                            if (num > maxPatternNum) maxPatternNum = num;
                        }
                    }
                }
                int nextPatternNum = maxPatternNum + 1;

                targetClip = new MidiPattern();
                targetClip->setName("Pattern " + juce::String(nextPatternNum));
                targetClip->setStartX(0.0f);
                targetClip->setWidth(1280.0f);
                targetClip->setColor(t.getColor());
                t.getMidiClips().add(targetClip);

                playlist.addMidiClipToView(&t, targetClip);
            }

            ui.setActiveClip(targetClip);

            if (onOpenPianoRoll) onOpenPianoRoll();
            };
    }

    static void connectPlaylist(PlaylistComponent& playlist,
        PianoRollComponent& ui,
        std::function<void()> onOpenPianoRoll)
    {
        playlist.onMidiClipDoubleClicked = [&ui, onOpenPianoRoll](Track* t, MidiPattern* clip) {
            if (t) t->migrateMidiToRelative(); // MIGRACIÓN AUTOMÁTICA
            ui.setActiveClip(clip);

            if (onOpenPianoRoll) onOpenPianoRoll();
            };
    }

    static void cleanup(PianoRollComponent& ui,
        std::function<void()> onClosePianoRoll,
        Track* trackToDelete)
    {
        MidiPattern* activeClip = ui.getActiveClip();
        if (activeClip != nullptr && trackToDelete->getMidiClips().contains(activeClip)) {
            ui.setActiveClip(nullptr);
            if (onClosePianoRoll) onClosePianoRoll();
        }
    }
};