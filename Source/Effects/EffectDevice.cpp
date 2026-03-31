#include "EffectDevice.h"
#include "EffectsPanel.h"
#include <memory>

EffectDevice::EffectDevice(int index, juce::String name, bool isInst, bool bypassed, BaseEffect* effectRef, EffectsPanel& p)
    : idx(index), fxName(name), isInstrument(isInst), isBypassed(bypassed), effect(effectRef), panel(p) {

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

    // --- NUEVO: CONFIGURACIÓN DEL BOTÓN DE RUTEO ---
    addAndMakeVisible(routingBtn);
    routingBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentWhite);
    routingBtn.setTooltip("Cambiar ruteo de procesamiento (L/R, Mid Only, Side Only)");
    updateRoutingButtonText();

    routingBtn.onClick = [this] {
        if (effect) {
            int current = (int)effect->getRouting();
            current = (current + 1) % 3; // Cicla entre 0=Stereo, 1=Mid, 2=Side
            effect->setRouting((PluginRouting)current);
            updateRoutingButtonText();
        }
        };

    // --- CLAVE: INCRUSTAR EDITOR NATIVO ---
    if (effectRef && effectRef->isNative()) {
        nativeEditor = effectRef->getCustomEditor();
        if (nativeEditor) {
            addAndMakeVisible(nativeEditor);
        }
    }

    // --- NUEVO: CONFIGURACIÓN DEL PICKER DE SIDECHAIN ---
    if (effect && effect->supportsSidechain()) {
        sidechainPicker = std::make_unique<SidechainPicker>([this](int sourceId) {
            if (effect) {
                effect->sidechainSourceTrackId.store(sourceId);
                // Notificar cambio de ruteo para relanzar el grafo
                if (panel.onReorderEffects && panel.getActiveTrack()) {
                    panel.onReorderEffects(*panel.getActiveTrack(), -1, -1); 
                }
            }
        });
        addAndMakeVisible(*sidechainPicker);
        updateSidechainPicker();
    }
}

void EffectDevice::updateSidechainPicker() {
    if (sidechainPicker && panel.getAvailableTracks) {
        auto tracks = panel.getAvailableTracks();
        sidechainPicker->refresh(panel.getActiveTrack(), tracks, effect->sidechainSourceTrackId.load());
    }
}

void EffectDevice::updateRoutingButtonText() {
    if (!effect) return;
    switch (effect->getRouting()) {
    case PluginRouting::Stereo:
        routingBtn.setButtonText("L/R");
        routingBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
        break;
    case PluginRouting::Mid:
        routingBtn.setButtonText("M");
        routingBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::orange);
        break;
    case PluginRouting::Side:
        routingBtn.setButtonText("S");
        routingBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::yellow);
        break;
    }
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

    // Acortamos el texto para que no pise el botón de ruteo
    g.drawText(fxName, headerArea.getX() + 28, headerArea.getY(), headerArea.getWidth() - 60, headerArea.getHeight(), juce::Justification::centred);

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawHorizontalLine(24, 0, getWidth());

    // Si NO es nativo, dibujamos el placeholder gris con texto
    if (!nativeEditor) {
        if (!isBypassed) {
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.setFont(juce::Font(12.0f, juce::Font::italic));
            g.drawText("External VST3\n\n(Doble clic\npara abrir)", area, juce::Justification::centred);

            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.drawEllipse(area.getCentreX() - 25, area.getCentreY() - 25, 50, 50, 3.0f);
        }
    }

    // El bypass pinta un overlay oscuro cubriendo el UI nativo o el texto
    if (isBypassed) {
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.fillRoundedRectangle(area.toFloat(), 4.0f);
        g.setColour(juce::Colours::darkgrey);
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText("BYPASS", area, juce::Justification::centred);
    }

    if (dragHoverMode == 1) {
        g.setColour(juce::Colours::orange);
        g.fillRoundedRectangle(2.0f, 2.0f, 4.0f, (float)getHeight() - 4.0f, 2.0f);
    }
    else if (dragHoverMode == 2) {
        g.setColour(juce::Colours::orange);
        g.fillRoundedRectangle((float)getWidth() - 6.0f, 2.0f, 4.0f, (float)getHeight() - 4.0f, 2.0f);
    }
}

void EffectDevice::resized() {
    auto area = getLocalBounds().reduced(2);
    auto headerArea = area.removeFromTop(24);

    bypassBtn.setBounds(headerArea.getX() + 4, headerArea.getY() + 3, 20, 18);
    
    int rightX = headerArea.getRight() - 4;
    
    // Botón de Ruteo (M/S/ST)
    rightX -= 24;
    routingBtn.setBounds(rightX, headerArea.getY() + 3, 24, 18);

    // Picker de Sidechain (si existe)
    if (sidechainPicker) {
        rightX -= 85; // Un poco más ancho para que se lea el track
        sidechainPicker->setBounds(rightX, headerArea.getY() + 3, 80, 18);
    }

    // El UI nativo ocupa todo el cuerpo restante de la "píldora" de efecto
    if (nativeEditor) {
        nativeEditor->setBounds(area);
    }
}

void EffectDevice::mouseDown(const juce::MouseEvent& e) {
    if (bypassBtn.getBounds().contains(e.getPosition()) || routingBtn.getBounds().contains(e.getPosition())) return;

    juce::Rectangle<int> area = getLocalBounds().reduced(2);
    juce::Rectangle<int> headerArea = area.removeFromTop(24);

    if (e.mods.isPopupMenu() && headerArea.contains(e.getPosition())) {
        juce::PopupMenu m;
        m.addItem(1, "Eliminar Plugin");
        m.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
            if (result == 1) {
                if (panel.onDeleteEffect && panel.getActiveTrack()) {
                    panel.onDeleteEffect(*panel.getActiveTrack(), idx);
                }
            }
            });
        return;
    }

    if (e.getNumberOfClicks() == 2) {
        if (panel.onOpenEffect && panel.getActiveTrack()) {
            panel.onOpenEffect(*panel.getActiveTrack(), idx);
        }
    }
}

void EffectDevice::mouseDrag(const juce::MouseEvent& e) {
    if (bypassBtn.getBounds().contains(e.getPosition()) || routingBtn.getBounds().contains(e.getPosition())) return;

    if (auto* dragContainer = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
        juce::Image snapshot = createComponentSnapshot(getLocalBounds());
        juce::Image ghost(juce::Image::ARGB, snapshot.getWidth(), snapshot.getHeight(), true);
        juce::Graphics g(ghost);
        g.setOpacity(0.6f);
        g.drawImageAt(snapshot, 0, 0);

        dragContainer->startDragging(dragID, this, ghost, true);
    }
}

bool EffectDevice::isInterestedInDragSource(const SourceDetails& details) {
    return details.description == dragID && details.sourceComponent != this;
}

void EffectDevice::itemDragEnter(const SourceDetails& details) {
    itemDragMove(details);
}

void EffectDevice::itemDragMove(const SourceDetails& details) {
    int oldMode = dragHoverMode;
    if (details.localPosition.x < getWidth() / 2) dragHoverMode = 1;
    else dragHoverMode = 2;
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
            if (finalMode == 2) targetIdx++;
            if (sourceDevice->idx < targetIdx) targetIdx--;
            panel.onReorderEffects(*panel.getActiveTrack(), sourceDevice->idx, targetIdx);
        }
    }
}