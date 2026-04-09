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

            for (auto* track : container.getTracks()) {
                bool trackModified = false;
                for (auto* targetClip : track->getMidiClips()) {
                    // Si es el clip original que estamos editando
                    if (targetClip == sourceClip) {
                        trackModified = true;
                    }
                    // Si es un clon (Ghost Copy) - Sincronizamos notas 0-based
                    else if (targetClip->getName() == sourceClip->getName()) {
                        targetClip->getNotes() = sourceClip->getNotes();

                        targetClip->autoVol = sourceClip->autoVol;
                        targetClip->autoPan = sourceClip->autoPan;
                        targetClip->autoPitch = sourceClip->autoPitch;
                        targetClip->autoFilter = sourceClip->autoFilter;

                        targetClip->setColor(sourceClip->getColor());
                        targetClip->setWidth(sourceClip->getWidth());
                        trackModified = true;
                    }
                }
                
                // CRUCIAL: Publicar los cambios al hilo de Audio (Doble Buffer)
                if (trackModified) {
                    track->commitSnapshot();
                }
            }
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
                targetClip->setWidth(320.0f);
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