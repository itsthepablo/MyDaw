#pragma once
#include <JuceHeader.h>
#include "../Playlist/PlaylistComponent.h"
#include "../PianoRoll/PianoRollComponent.h"

class TrackPianoRollBridge {
public:
    static void connect(TrackContainer& container,
        PlaylistComponent& playlist,
        PianoRollComponent& ui,
        std::function<void()> onOpenPianoRoll)
    {
        container.onOpenPianoRoll = [&playlist, &ui, onOpenPianoRoll](Track& t) {
            MidiClipData* targetClip = nullptr;

            // Buscamos si la pista ya tiene algún patrón dibujado
            if (t.midiClips.size() > 0) {
                targetClip = t.midiClips.getFirst();
            }
            else {
                // Si la pista está vacía, le hacemos el favor al usuario y creamos "Pattern 1"
                targetClip = new MidiClipData();
                targetClip->name = "Pattern 1";
                targetClip->startX = 0.0f;
                targetClip->width = 320.0f;
                targetClip->color = t.getColor();
                t.midiClips.add(targetClip);

                // Le decimos a la Playlist que dibuje este nuevo bloque
                playlist.addMidiClipToView(&t, targetClip);
            }

            // Le inyectamos las notas exclusivas del clip al Piano Roll
            ui.setActiveNotes(&(targetClip->notes));

            // Disparamos el Modo Enfoque en el DAW
            if (onOpenPianoRoll) onOpenPianoRoll();
            };
    }

    static void connectPlaylist(PlaylistComponent& playlist,
        PianoRollComponent& ui,
        std::function<void()> onOpenPianoRoll)
    {
        playlist.onMidiClipDoubleClicked = [&ui, onOpenPianoRoll](Track* t, MidiClipData* clip) {
            ui.setActiveNotes(&(clip->notes));

            // Disparamos el Modo Enfoque en el DAW
            if (onOpenPianoRoll) onOpenPianoRoll();
            };
    }

    static void cleanup(PianoRollComponent& ui,
        std::function<void()> onClosePianoRoll,
        Track* trackToDelete)
    {
        if (ui.getActiveNotesPointer() == &(trackToDelete->notes)) {
            ui.setActiveNotes(nullptr);
            if (onClosePianoRoll) onClosePianoRoll();
        }
    }
};