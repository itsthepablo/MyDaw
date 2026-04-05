#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
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
        auto syncEngine = [&container, &playlist](MidiClipData* sourceClip) {
            if (!sourceClip) return;

            for (auto* track : container.getTracks()) {
                bool trackModified = false;
                for (auto* targetClip : track->midiClips) {
                    // Si es el clip original que estamos editando
                    if (targetClip == sourceClip) {
                        trackModified = true;
                    }
                    // Si es un clon (Ghost Copy) - Sincronizamos notas
                    else if (targetClip->name == sourceClip->name) {
                        float timeShift = targetClip->startX - sourceClip->startX;
                        targetClip->notes = sourceClip->notes;
                        for (auto& note : targetClip->notes) note.x += timeShift;

                        targetClip->autoVol = sourceClip->autoVol;
                        targetClip->autoPan = sourceClip->autoPan;
                        targetClip->autoPitch = sourceClip->autoPitch;
                        targetClip->autoFilter = sourceClip->autoFilter;

                        auto shiftLane = [timeShift](AutoLane& lane) {
                            for (auto& node : lane.nodes) node.x += timeShift;
                            };
                        shiftLane(targetClip->autoVol);
                        shiftLane(targetClip->autoPan);
                        shiftLane(targetClip->autoPitch);
                        shiftLane(targetClip->autoFilter);

                        targetClip->color = sourceClip->color;
                        targetClip->width = sourceClip->width;
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
            MidiClipData* targetClip = nullptr;

            if (t.midiClips.size() > 0) {
                targetClip = t.midiClips.getFirst();
            }
            else {
                int maxPatternNum = 0;
                for (auto* tr : container.getTracks()) {
                    for (auto* mc : tr->midiClips) {
                        if (mc->name.startsWith("Pattern ")) {
                            int num = mc->name.substring(8).getIntValue();
                            if (num > maxPatternNum) maxPatternNum = num;
                        }
                    }
                }
                int nextPatternNum = maxPatternNum + 1;

                targetClip = new MidiClipData();
                targetClip->name = "Pattern " + juce::String(nextPatternNum);
                targetClip->startX = 0.0f;
                targetClip->width = 320.0f;
                targetClip->color = t.getColor();
                t.midiClips.add(targetClip);

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
        playlist.onMidiClipDoubleClicked = [&ui, onOpenPianoRoll](Track* t, MidiClipData* clip) {
            ui.setActiveClip(clip);

            if (onOpenPianoRoll) onOpenPianoRoll();
            };
    }

    static void cleanup(PianoRollComponent& ui,
        std::function<void()> onClosePianoRoll,
        Track* trackToDelete)
    {
        MidiClipData* activeClip = ui.getActiveClip();
        if (activeClip != nullptr && trackToDelete->midiClips.contains(activeClip)) {
            ui.setActiveClip(nullptr);
            if (onClosePianoRoll) onClosePianoRoll();
        }
    }
};