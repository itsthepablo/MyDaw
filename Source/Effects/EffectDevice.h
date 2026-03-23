#pragma once
#include <JuceHeader.h>
#include <functional>

class EffectsPanel;

// ==============================================================================
// Componente que representa una ranura o 'caja' de efecto individual
// ==============================================================================
class EffectDevice : public juce::Component, public juce::DragAndDropTarget {
public:
    EffectDevice(int index, juce::String name, bool isInst, bool isBypassed, EffectsPanel& p);

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    // --- DragAndDropTarget ---
    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDragEnter(const SourceDetails& details) override;

    // --- NUEVO: Detecta el movimiento interno para saber a que lado soltar ---
    void itemDragMove(const SourceDetails& details) override;

    void itemDragExit(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;

private:
    int idx;
    juce::String fxName;
    bool isInstrument;
    bool isBypassed;
    EffectsPanel& panel;

    // --- Visuals ---
    juce::TextButton bypassBtn;

    // 0 = Ninguno, 1 = Mitad Izquierda, 2 = Mitad Derecha
    int dragHoverMode = 0;

    // Identificador unico para el arrastre
    const juce::String dragID = "EFFECT_DEVICE_SLOT";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectDevice)
};