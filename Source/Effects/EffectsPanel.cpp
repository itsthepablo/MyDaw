#include "EffectsPanel.h"
#include "../PluginHost/VSTHost.h"

std::map<void*, bool> EffectsPanel::pluginIsInstrumentMap;

// ==============================================================================
// EFFECT SLOT (Cada fila de la lista de efectos)
// ==============================================================================

EffectSlot::EffectSlot(int index, juce::String name, bool isInst, EffectsPanel& p)
    : idx(index), fxName(name), isInstrument(isInst), panel(p) {
}

void EffectSlot::paint(juce::Graphics& g) {
    auto area = getLocalBounds().reduced(2);

    if (isDragHovering) {
        g.setColour(juce::Colour(70, 75, 80));
    }
    else {
        g.setColour(juce::Colour(45, 48, 53));
    }
    g.fillRoundedRectangle(area.toFloat(), 4.0f);

    g.setColour(isInstrument ? juce::Colours::orange : juce::Colours::dodgerblue);
    g.fillRoundedRectangle(8.0f, 6.0f, 18.0f, 18.0f, 3.0f);

    g.setColour(juce::Colours::white);
    g.drawVerticalLine(17, 9.0f, 14.0f);

    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(14.0f);
    g.drawText(fxName, 35, 0, getWidth() - 40, getHeight(), juce::Justification::centredLeft);
}

void EffectSlot::mouseDown(const juce::MouseEvent& e) {
    if (e.getNumberOfClicks() == 2) {
        if (panel.onOpenEffect && panel.getActiveTrack()) {
            panel.onOpenEffect(*panel.getActiveTrack(), idx);
        }
    }
}

void EffectSlot::mouseDrag(const juce::MouseEvent& e) {
    if (auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
        // CORRECCIÓN RADICAL: Etiquetas 100% distintas. Nunca se podrán mezclar.
        juce::String dragID = isInstrument ? "INSTRUMENT_SLOT" : "FX_SLOT";
        dragContainer->startDragging(dragID, this, juce::Image(), true);
    }
}

bool EffectSlot::isInterestedInDragSource(const SourceDetails& details) {
    // Si la etiqueta no coincide con mi tipo, rechazo el arrastre de inmediato.
    juce::String expectedID = isInstrument ? "INSTRUMENT_SLOT" : "FX_SLOT";
    return details.description == expectedID && details.sourceComponent != this;
}

void EffectSlot::itemDragEnter(const SourceDetails& details) {
    isDragHovering = true;
    repaint();
}

void EffectSlot::itemDragExit(const SourceDetails& details) {
    isDragHovering = false;
    repaint();
}

void EffectSlot::itemDropped(const SourceDetails& details) {
    isDragHovering = false;
    repaint();

    if (auto* sourceSlot = dynamic_cast<EffectSlot*>(details.sourceComponent.get())) {
        if (panel.onReorderEffects && panel.getActiveTrack()) {
            panel.onReorderEffects(*panel.getActiveTrack(), sourceSlot->idx, this->idx);
        }
    }
}


// ==============================================================================
// EFFECTS PANEL (Panel Lateral Principal)
// ==============================================================================

EffectsPanel::EffectsPanel() {
    addAndMakeVisible(closeBtn);
    closeBtn.setButtonText("X");
    closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentWhite);
    closeBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.7f));
    closeBtn.onClick = [this] { if (onClose) onClose(); };

    addAndMakeVisible(addEffectBtn);
    addEffectBtn.setButtonText("Add effect");
    addEffectBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(70, 75, 80));
    addEffectBtn.onClick = [this] { if (onAddEffect && activeTrack) onAddEffect(*activeTrack); };
}

void EffectsPanel::setTrack(Track* t) {
    activeTrack = t;
    updateSlots();
}

void EffectsPanel::updateSlots() {
    slots.clear();
    if (activeTrack) {
        for (int i = 0; i < activeTrack->plugins.size(); ++i) {
            auto* host = activeTrack->plugins[i];

            // Si es un track de AUDIO, forzamos a que todo sea considerado Efecto (isInst = false)
            bool isInst = false;
            if (activeTrack->getType() == TrackType::MIDI) {
                isInst = pluginIsInstrumentMap[(void*)host];
            }

            juce::String name = isInst ? "VSTi Plugin" : "VST3 Plugin";

            if (host != nullptr && host->isLoaded()) {
                name = host->getLoadedPluginName();
            }
            auto* slot = new EffectSlot(i, name, isInst, *this);
            slots.add(slot);
            addAndMakeVisible(slot);
        }
    }
    resized();
    repaint();
}

void EffectsPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(40, 43, 48));

    g.setColour(juce::Colour(30, 33, 36));
    g.fillRect(0, 0, getWidth(), 40);

    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(16.0f));
    g.drawText("Realtime effects", 15, 0, getWidth() - 50, 40, juce::Justification::centredLeft);

    g.setColour(juce::Colour(45, 48, 53));
    g.fillRect(0, 40, getWidth(), 40);

    if (activeTrack) {
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(15.0f, juce::Font::bold));
        g.drawText(activeTrack->getName(), 40, 40, getWidth() - 50, 40, juce::Justification::centredLeft);

        g.setColour(activeTrack->getColor());
        g.fillEllipse(15, 52, 14, 14);
    }

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawVerticalLine(getWidth() - 1, 0.0f, (float)getHeight());

    // --- DIBUJO EXPLÍCITO DE SUBTÍTULOS BASADO EN TRACKTYPE ---
    if (activeTrack) {
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.setFont(12.0f);

        // El bloque de instrumentos solo se dibuja si es pista MIDI
        if (activeTrack->getType() == TrackType::MIDI) {
            g.drawText("INSTRUMENTS", 15, instHeaderY, getWidth() - 30, 20, juce::Justification::bottomLeft);
        }
        g.drawText("EFFECTS / VST3", 15, fxHeaderY, getWidth() - 30, 20, juce::Justification::bottomLeft);
    }
}

void EffectsPanel::resized() {
    closeBtn.setBounds(getWidth() - 35, 5, 30, 30);
    if (!activeTrack) return;

    int y = 90;

    // 1. BLOQUE DE INSTRUMENTOS (Exclusivo para pistas MIDI)
    if (activeTrack->getType() == TrackType::MIDI) {
        instHeaderY = y;
        y += 25; // Espacio para el título "INSTRUMENTS"

        for (auto* slot : slots) {
            if (slot->isInstrument) {
                slot->setBounds(10, y, getWidth() - 20, 30);
                y += 32;
            }
        }
        y += 10; // Separación antes del bloque de Efectos
    }

    // 2. BLOQUE DE EFECTOS (Para todo tipo de pista)
    fxHeaderY = y;
    y += 25; // Espacio para el título "EFFECTS / VST3"

    for (auto* slot : slots) {
        if (!slot->isInstrument) {
            slot->setBounds(10, y, getWidth() - 20, 30);
            y += 32;
        }
    }

    addEffectBtn.setBounds(15, y + 10, getWidth() - 30, 30);
}