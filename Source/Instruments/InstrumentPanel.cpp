#include "InstrumentPanel.h"

InstrumentPanel::InstrumentPanel() {
    addAndMakeVisible(addInstrumentBtn);
    addInstrumentBtn.setButtonText("+ ADD INSTRUMENT (VSTi)");
    addInstrumentBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 65, 70));
    addInstrumentBtn.onClick = [this] {
        if (activeTrack != nullptr && onAddInstrument) {
            onAddInstrument(*activeTrack);
        }
        };

    addAndMakeVisible(openInstrumentBtn);
    openInstrumentBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 100, 150));
    openInstrumentBtn.onClick = [this] {
        if (activeTrack != nullptr && currentInstrument != nullptr && onOpenInstrumentWindow) {
            onOpenInstrumentWindow(*activeTrack, currentInstrument);
        }
        };
}

InstrumentPanel::~InstrumentPanel() {
}

void InstrumentPanel::setTrack(Track* t) {
    activeTrack = t;
    updateInstrumentView();
}

void InstrumentPanel::updateInstrumentView() {
    currentInstrument = nullptr;

    // Buscar si la pista ya tiene un VSTi cargado usando el mapa global de EffectsPanel
    if (activeTrack != nullptr) {
        for (auto* p : activeTrack->plugins) {
            if (EffectsPanel::pluginIsInstrumentMap[(void*)p]) {
                currentInstrument = p;
                break;
            }
        }
    }

    if (currentInstrument != nullptr) {
        addInstrumentBtn.setVisible(false);
        openInstrumentBtn.setVisible(true);
        openInstrumentBtn.setButtonText("OPEN: " + currentInstrument->getLoadedPluginName());
    }
    else if (activeTrack != nullptr) {
        addInstrumentBtn.setVisible(true);
        openInstrumentBtn.setVisible(false);
    }
    else {
        addInstrumentBtn.setVisible(false);
        openInstrumentBtn.setVisible(false);
    }

    resized();
    repaint();
}

void InstrumentPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(20, 22, 25)); // Fondo del BottomDock

    if (activeTrack == nullptr) {
        g.setColour(juce::Colours::grey);
        g.drawText("Selecciona una pista MIDI para cargar un Instrumento.", getLocalBounds(), juce::Justification::centred, true);
    }
    else {
        g.setColour(juce::Colours::white);
        g.drawText("INSTRUMENT PANEL - Pista: " + activeTrack->getName(), getLocalBounds().removeFromTop(40), juce::Justification::centred, true);
    }
}

void InstrumentPanel::resized() {
    if (activeTrack != nullptr) {
        auto area = getLocalBounds();
        area.removeFromTop(50); // Dejamos espacio para el texto superior

        if (currentInstrument != nullptr) {
            openInstrumentBtn.setBounds(area.removeFromTop(40).withSizeKeepingCentre(250, 40));
        }
        else {
            addInstrumentBtn.setBounds(area.removeFromTop(30).withSizeKeepingCentre(220, 30));
        }
    }
}