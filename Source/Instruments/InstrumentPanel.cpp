#include "InstrumentPanel.h"

InstrumentPanel::InstrumentPanel() {
    addAndMakeVisible(addInstrumentBtn);
    // Texto m�s limpio para que encaje en la "caja"
    addInstrumentBtn.setButtonText("+ ADD VSTi");

    // Colores tipo "Slot vac�o" de Ableton
    addInstrumentBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 43, 48));
    addInstrumentBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));

    addInstrumentBtn.onClick = [this] {
        if (activeTrack != nullptr && onAddInstrument) {
            onAddInstrument(*activeTrack);
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
    instrumentButtons.clear();

    if (activeTrack != nullptr) {
        for (auto* p : activeTrack->plugins) {
            if (EffectsPanel::pluginIsInstrumentMap[(void*)p]) {
                // Creamos una caja para cada instrumento
                auto* btn = new juce::TextButton(p->getLoadedPluginName());

                // Dise�o de la caja (Ableton style)
                btn->setColour(juce::TextButton::buttonColourId, juce::Colour(55, 60, 65));
                btn->setColour(juce::TextButton::buttonOnColourId, juce::Colour(75, 80, 85));
                btn->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
                btn->setTooltip("Clic para abrir la ventana del sintetizador");

                btn->onClick = [this, p] {
                    if (activeTrack != nullptr && onOpenInstrumentWindow) {
                        onOpenInstrumentWindow(*activeTrack, p);
                    }
                    };

                instrumentButtons.add(btn);
                addAndMakeVisible(btn);
            }
        }
        addInstrumentBtn.setVisible(true);
    }
    else {
        addInstrumentBtn.setVisible(false);
    }

    resized();
    repaint();
}

void InstrumentPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(30, 33, 36)); // Fondo principal un poco m�s oscuro

    if (activeTrack == nullptr) {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText("SELECCIONA UNA PISTA MIDI", getLocalBounds(), juce::Justification::centred, true);
    }
    else {
        // Etiqueta sutil en la esquina superior izquierda
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText("INSTRUMENT CHAIN: " + activeTrack->getName(), 15, 10, 300, 20, juce::Justification::centredLeft);
    }
}

void InstrumentPanel::resized() {
    if (activeTrack != nullptr) {
        auto area = getLocalBounds();

        // Dejamos un margen arriba para el texto y m�rgenes a los lados
        area.removeFromTop(35);
        area.removeFromBottom(15);
        area.removeFromLeft(15);
        area.removeFromRight(15);

        int boxWidth = 140; // Ancho fijo de la caja tipo Ableton
        int padding = 10;   // Espacio entre cajas

        // Colocamos las cajas de los VSTi de izquierda a derecha
        for (auto* btn : instrumentButtons) {
            btn->setBounds(area.removeFromLeft(boxWidth));
            area.removeFromLeft(padding); // Separador
        }

        // Colocamos el bot�n de agregar al final de la cadena
        addInstrumentBtn.setBounds(area.removeFromLeft(boxWidth));
    }
}