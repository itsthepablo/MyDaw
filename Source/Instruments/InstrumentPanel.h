#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include "../Effects/EffectsPanel.h" // Necesario para acceder a pluginIsInstrumentMap

class InstrumentPanel : public juce::Component {
public:
    InstrumentPanel();
    ~InstrumentPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setTrack(Track* t);
    void updateInstrumentView(); // <-- NUEVO: Actualiza la vista si ya hay un VSTi cargado

    // Callbacks que enviaremos al Bridge/MainComponent
    std::function<void(Track&)> onAddInstrument;
    std::function<void(Track&, BaseEffect*)> onOpenInstrumentWindow;

private:
    Track* activeTrack = nullptr;
    juce::TextButton addInstrumentBtn;
    juce::TextButton openInstrumentBtn; // <-- NUEVO: Botara reabrir la ventana del VSTi
    BaseEffect* currentInstrument = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentPanel)
};