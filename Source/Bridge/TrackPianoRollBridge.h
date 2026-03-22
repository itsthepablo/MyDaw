#pragma once
#include <JuceHeader.h>
#include "../Playlist/PlaylistComponent.h"
#include "../PianoRoll/PianoRollComponent.h"

class PianoRollWindow : public juce::DocumentWindow {
public:
    PianoRollWindow(juce::String name, juce::Component* c)
        : DocumentWindow(name, juce::Colours::black, allButtons) {
        setUsingNativeTitleBar(true);
        setContentNonOwned(c, true);
        setResizable(true, true);
        centreWithSize(800, 600);
    }
    void closeButtonPressed() override { setVisible(false); }
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollWindow)
};

class TrackPianoRollBridge {
public:
    // NUEVA LÓGICA: El botón abre el primer Pattern, o crea uno si está vacío
    static void connect(TrackContainer& container,
        PlaylistComponent& playlist,
        PianoRollComponent& ui,
        std::unique_ptr<juce::DocumentWindow>& window)
    {
        container.onOpenPianoRoll = [&playlist, &ui, &window](Track& t) {
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

            if (!window) {
                window = std::make_unique<PianoRollWindow>(targetClip->name, &ui);
            }

            window->setName("Piano Roll: " + t.getName() + " - " + targetClip->name);
            window->setVisible(true);
            window->toFront(true);
            };
    }

    static void connectPlaylist(PlaylistComponent& playlist,
        PianoRollComponent& ui,
        std::unique_ptr<juce::DocumentWindow>& window)
    {
        playlist.onMidiClipDoubleClicked = [&ui, &window](Track* t, MidiClipData* clip) {
            ui.setActiveNotes(&(clip->notes));

            if (!window) {
                window = std::make_unique<PianoRollWindow>(clip->name, &ui);
            }

            window->setName("Piano Roll: " + t->getName() + " - " + clip->name);
            window->setVisible(true);
            window->toFront(true);
            };
    }

    static void cleanup(PianoRollComponent& ui,
        std::unique_ptr<juce::DocumentWindow>& window,
        Track* trackToDelete)
    {
        if (ui.getActiveNotesPointer() == &(trackToDelete->notes)) {
            ui.setActiveNotes(nullptr);
            if (window) window->setVisible(false);
        }
    }
};