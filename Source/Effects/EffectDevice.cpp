#include "EffectDevice.h"
#include "EffectsPanel.h"

EffectDevice::EffectDevice(int index, juce::String name, bool isInst, bool bypassed, EffectsPanel& p)
    : idx(index), fxName(name), isInstrument(isInst), isBypassed(bypassed), panel(p) {

    addAndMakeVisible(bypassBtn);
    bypassBtn.setButtonText("B");
    bypassBtn.setClickingTogglesState(true);
    bypassBtn.setToggleState(!bypassed, juce::dontSendNotification);

    bypassBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentWhite);
    bypassBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::dodgerblue);
    bypassBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::darkgrey);
    bypassBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    bypassBtn.onClick = [this] {
        if (panel.onBypassChanged && panel.getActiveTrack()) {
            panel.onBypassChanged(*panel.getActiveTrack(), idx, !bypassBtn.getToggleState());
        }
        };
}

void EffectDevice::paint(juce::Graphics& g) {
    auto area = getLocalBounds().reduced(2);

    g.setColour(juce::Colour(45, 48, 53));
    g.fillRoundedRectangle(area.toFloat(), 6.0f);

    auto headerArea = area.removeFromTop(24);

    juce::Path headerPath;
    headerPath.addRoundedRectangle(headerArea.getX(), headerArea.getY(), headerArea.getWidth(), headerArea.getHeight() + 6, 6.0f, 6.0f, true, true, false, false);
    g.setColour(juce::Colour(30, 33, 36));
    g.fillPath(headerPath);

    g.setColour(isInstrument ? juce::Colours::orange : juce::Colours::dodgerblue);
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText(fxName, headerArea.getX() + 35, headerArea.getY(), headerArea.getWidth() - 40, headerArea.getHeight(), juce::Justification::centredLeft);

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawHorizontalLine(24, 0, getWidth());

    if (!isBypassed) {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.setFont(juce::Font(12.0f, juce::Font::italic));
        g.drawText("External VST3\n\n(Doble clic\npara abrir)", area, juce::Justification::centred);

        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawEllipse(area.getCentreX() - 25, area.getCentreY() - 25, 50, 50, 3.0f);
    }
    else {
        g.fillAll(juce::Colours::black.withAlpha(0.4f));
        g.setColour(juce::Colours::darkgrey);
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText("BYPASS", area, juce::Justification::centred);
    }

    // --- NUEVO: INDICADOR VISUAL DE INSERCION (Lineas Naranjas) ---
    if (dragHoverMode == 1) {
        // Linea gruesa en el borde izquierdo
        g.setColour(juce::Colours::orange);
        g.fillRoundedRectangle(2.0f, 2.0f, 4.0f, (float)getHeight() - 4.0f, 2.0f);
    }
    else if (dragHoverMode == 2) {
        // Linea gruesa en el borde derecho
        g.setColour(juce::Colours::orange);
        g.fillRoundedRectangle((float)getWidth() - 6.0f, 2.0f, 4.0f, (float)getHeight() - 4.0f, 2.0f);
    }
}

void EffectDevice::resized() {
    auto area = getLocalBounds().reduced(2);
    auto headerArea = area.removeFromTop(24);
    bypassBtn.setBounds(headerArea.getX() + 6, headerArea.getY() + 3, 20, 18);
}

void EffectDevice::mouseDown(const juce::MouseEvent& e) {
    if (bypassBtn.getBounds().contains(e.getPosition())) return;

    if (e.getNumberOfClicks() == 2) {
        if (panel.onOpenEffect && panel.getActiveTrack()) {
            panel.onOpenEffect(*panel.getActiveTrack(), idx);
        }
    }
}

void EffectDevice::mouseDrag(const juce::MouseEvent& e) {
    if (bypassBtn.getBounds().contains(e.getPosition())) return;

    if (auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this)) {

        // --- NUEVO: CREAR LA IMAGEN FANTASMA (GHOST) ---
        // Tomamos una "fotografia" del componente exacto como se ve ahora
        juce::Image snapshot = createComponentSnapshot(getLocalBounds());

        // Creamos una nueva imagen con transparencia (opacidad al 60%)
        juce::Image ghost(juce::Image::ARGB, snapshot.getWidth(), snapshot.getHeight(), true);
        juce::Graphics g(ghost);
        g.setOpacity(0.6f);
        g.drawImageAt(snapshot, 0, 0);

        // Le pasamos la imagen fantasma al contenedor de arrastre
        dragContainer->startDragging(dragID, this, ghost, true);
    }
}

// ==============================================================================
// Drag & Drop (Reordenacion de la cadena)
// ==============================================================================

bool EffectDevice::isInterestedInDragSource(const SourceDetails& details) {
    return details.description == dragID && details.sourceComponent != this;
}

void EffectDevice::itemDragEnter(const SourceDetails& details) {
    itemDragMove(details);
}

void EffectDevice::itemDragMove(const SourceDetails& details) {
    int oldMode = dragHoverMode;

    // Si el raton esta en la mitad izquierda, iluminamos la izquierda (insertar ANTES)
    // Si esta en la mitad derecha, iluminamos la derecha (insertar DESPUES)
    if (details.localPosition.x < getWidth() / 2) {
        dragHoverMode = 1;
    }
    else {
        dragHoverMode = 2;
    }

    // Solo repintamos si cambiamos de mitad para ahorrar CPU
    if (oldMode != dragHoverMode) repaint();
}

void EffectDevice::itemDragExit(const SourceDetails& details) {
    dragHoverMode = 0;
    repaint();
}

void EffectDevice::itemDropped(const SourceDetails& details) {
    int finalMode = dragHoverMode;
    dragHoverMode = 0;
    repaint();

    if (auto* sourceDevice = dynamic_cast<EffectDevice*>(details.sourceComponent.get())) {
        if (panel.onReorderEffects && panel.getActiveTrack()) {
            int targetIdx = this->idx;

            // Ajuste matematico de indices
            if (finalMode == 2) {
                targetIdx++; // Insertar DESPUES de esta caja
            }

            if (sourceDevice->idx < targetIdx) {
                targetIdx--; // Compensar el desplazamiento si venia de atras
            }

            panel.onReorderEffects(*panel.getActiveTrack(), sourceDevice->idx, targetIdx);
        }
    }
}