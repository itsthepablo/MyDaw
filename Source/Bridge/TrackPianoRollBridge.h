#pragma once
#include <JuceHeader.h>

// La clase se queda igual
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
    // CAMBIO: Ahora usamos juce::DocumentWindow en lugar de PianoRollWindow
    static void connect(TrackContainer& container,
        PianoRollComponent& ui,
        std::unique_ptr<juce::DocumentWindow>& window)
    {
        container.onOpenPianoRoll = [&ui, &window](Track& t) {
            ui.setActiveNotes(&(t.notes));

            if (!window) {
                // Aquí sí podemos crear el PianoRollWindow específico 
                // porque un DocumentWindow puede guardar a su hijo.
                window = std::make_unique<PianoRollWindow>(t.getName(), &ui);
            }

            window->setName("Piano Roll: " + t.getName());
            window->setVisible(true);
            window->toFront(true);
            };
    }

    // CAMBIO: También aquí usamos el tipo base juce::DocumentWindow
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