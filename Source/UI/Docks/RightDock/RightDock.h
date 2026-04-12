#pragma once
#include <JuceHeader.h>
#include "../../Panels/SelectedTrack/SelectedTrackPanel.h"
#include "../../Meters/LevelMeter.h"

/**
 * RightDock: El contenedor del lado derecho de la pantalla.
 * Aloja el panel de "Selected Track" y gestiona su tamano.
 */
class RightDock : public juce::Component {
public:
    RightDock() {
        addAndMakeVisible(selectedTrackPanel);
        
        // --- 1. BOTÓN PARA EXPANDIR (Abajo en el eje Z) ---
        addAndMakeVisible(expandBtn);
        expandBtn.setButtonText("<");
        expandBtn.setOpaque(false);
        expandBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        expandBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white.withAlpha(0.1f));
        expandBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.7f));
        expandBtn.setTooltip("Mostrar Inspector");
        expandBtn.onClick = [this] {
            isExpanded = true;
            if (onLayoutChanged) onLayoutChanged();
        };

        // --- 2. MEDIDOR DE PICOS (Arriba en el eje Z, pero traspasa clics) ---
        addAndMakeVisible(collapsedMeter);
        collapsedMeter.setTooltip("Nivel del Track Seleccionado");
        collapsedMeter.setInterceptsMouseClicks(false, false); // El raton "traspasa" el metro y llega al boton

        // --- BOTÓN PARA CONTRAER (Texto HIDE cuando está abierto) ---
        addAndMakeVisible(hideBtn);
        hideBtn.setButtonText("HIDE");
        hideBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        hideBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.4f));
        hideBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        hideBtn.setTooltip("Ocultar Inspector");
        hideBtn.onClick = [this] {
            isExpanded = false;
            if (onLayoutChanged) onLayoutChanged();
        };

        updateVisibility();
    }

    ~RightDock() override = default;

    /**
     * Devuelve el ancho idoneo.
     * 150px si está expandido, 25px si está colapsado (estilo GainStation).
     */
    int getIdealWidthForHeight(int /*h*/) const {
        return isExpanded ? 150 : 25;
    }

    void setTrack(Track* t) {
        selectedTrackPanel.setTrack(t);
        collapsedMeter.setTrack(t);
    }

    void paint(juce::Graphics& g) override {
        if (!isExpanded) {
            // Fondo oscuro estilo GainStation para la barra lateral
            g.fillAll(juce::Colour(15, 18, 20));
        }
        else {
            // Fondo estandar del dock
            g.fillAll(juce::Colour(20, 22, 25));
            
            // Borde izquierdo para separar de la Playlist
            g.setColour(juce::Colour(45, 48, 52));
            g.drawVerticalLine(0, 0, (float)getHeight());
        }
    }

    void resized() override {
        updateVisibility();
        auto area = getLocalBounds();

        if (isExpanded) {
            auto topArea = area.removeFromTop(20);
            hideBtn.setBounds(topArea.removeFromRight(50).reduced(2));
            
            area.removeFromLeft(1); // Borde
            selectedTrackPanel.setBounds(area);
        }
        else {
            // Ambas ocupan toda la altura, el botón está abajo pero recibe clics
            expandBtn.setBounds(area);
            collapsedMeter.setBounds(area.reduced(2, 5)); // Mas ancho para que se vea mejor (solo 2px de margen lateral)
        }
    }

    void updateVisibility() {
        selectedTrackPanel.setVisible(isExpanded);
        hideBtn.setVisible(isExpanded);
        expandBtn.setVisible(!isExpanded);
        collapsedMeter.setVisible(!isExpanded);
    }

    bool isExpanded = true;
    std::function<void()> onLayoutChanged;
    SelectedTrackPanel selectedTrackPanel;
    LevelMeter collapsedMeter;

private:
    juce::TextButton expandBtn, hideBtn;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RightDock)
};
