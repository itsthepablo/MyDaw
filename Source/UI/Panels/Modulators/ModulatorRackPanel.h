#pragma once
#include <JuceHeader.h>
#include "../../Modulators/ModulatorDeviceUI.h"
#include "../../../Engine/Core/TransportState.h"
#include "ModMappingList.h"
#include "../../../Data/Track.h"
#include "../../../Engine/Modulation/GridModulator.h"

/**
 * ModulatorRackPanel: Panel dividido con lista de mapeos (izq) y rack de dispositivos (der).
 */
class ModulatorRackPanel : public juce::Component, public juce::ChangeListener {
public:
    ModulatorRackPanel() {
        addAndMakeVisible(mappingList);
        
        addAndMakeVisible(addModBtn);
        addModBtn.setButtonText("+ ADD MODULATOR");
        addModBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(50, 55, 60));
        addModBtn.onClick = [this] { addModulator(); };

        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&container);
        viewport.setScrollBarThickness(10);
        // Forzar scroll vertical, ocultar horizontal
        viewport.setScrollBarsShown(true, false, true, true);
    }

    void setTransportState(TransportState* ts) { transportState = ts; }

    void setTrack(Track* t) {
        if (currentTrack == t) return;
        
        if (currentTrack) currentTrack->removeChangeListener(this);
        currentTrack = t;
        if (currentTrack) currentTrack->addChangeListener(this);
        
        mappingList.setTrack(t);
        refresh();
    }

    void changeListenerCallback(juce::ChangeBroadcaster*) override {
        refresh();
    }

    void refresh() {
        modulatorUIs.clear();
        container.removeAllChildren();

        if (currentTrack) {
            for (auto* m : currentTrack->modulators) {
                auto* ui = new ModulatorDeviceUI(*m, transportState);
                modulatorUIs.add(ui);
                container.addAndMakeVisible(ui);
                ui->setInterceptsMouseClicks(true, true);
            }
        }
        
        // Actualizar tamaño del contenedor antes de reposicionar
        updateContainerSize();
    }

    void addModulator() {
        if (currentTrack) {
            currentTrack->modulators.add(new GridModulator());
            refresh();
        }
    }

    void updateContainerSize() {
        int rowHeight = 160;
        int w = std::max(100, viewport.getWidth() - viewport.getScrollBarThickness());
        int h = std::max(100, (int)modulatorUIs.size() * rowHeight);
        container.setSize(w, h);
        resized(); // Gatillo para reposicionar hijos
    }

    void resized() override {
        auto area = getLocalBounds();
        
        // 1. Lista de Mapeos a la Izquierda
        mappingList.setBounds(area.removeFromLeft(250));
        
        // 2. Rack a la Derecha
        auto rightArea = area;
        auto top = rightArea.removeFromTop(40);
        addModBtn.setBounds(top.removeFromRight(150).reduced(5));

        viewport.setBounds(rightArea);
        
        int rowHeight = 160;
        int width = container.getWidth();

        for (int i = 0; i < modulatorUIs.size(); ++i) {
            modulatorUIs[i]->setBounds(10, i * rowHeight + 5, width - 20, rowHeight - 10);
        }
    }

private:
    TransportState* transportState = nullptr;
    Track* currentTrack = nullptr;
    ModMappingList mappingList;
    juce::Viewport viewport;
    juce::Component container;
    juce::TextButton addModBtn;
    juce::OwnedArray<ModulatorDeviceUI> modulatorUIs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorRackPanel)
};
