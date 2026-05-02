#pragma once
#include <JuceHeader.h>
#include <functional>
#include "../../../../NativePlugins/BaseEffect.h"

class EffectsPanel;

// ==============================================================================
// Componente que representa una ranura o 'caja' de efecto individual
// ==============================================================================
class EffectDevice : public juce::Component, public juce::DragAndDropTarget {
public:
    EffectDevice(int index, juce::String name, bool isInst, bool isBypassed, BaseEffect* effectRef, EffectsPanel& p);

    void paint(juce::Graphics& g) override;
    void resized() override;
    int getPreferredWidth() const { return nativeEditor ? 400 : 140; }
    int getPreferredHeight() const { return nativeEditor ? 220 : 120; }
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    // --- DragAndDropTarget ---
    bool isInterestedInDragSource(const SourceDetails& details) override;
    void itemDragEnter(const SourceDetails& details) override;
    void itemDragMove(const SourceDetails& details) override;
    void itemDragExit(const SourceDetails& details) override;
    void itemDropped(const SourceDetails& details) override;

private:
    int idx;
    juce::String fxName;
    bool isInstrument;
    bool isBypassed;
    BaseEffect* effect; // Referencia al plugin para cambiar su ruteo
    EffectsPanel& panel;

    // --- Puntero al editor embebido (solo si es Nativo) ---
    juce::Component* nativeEditor = nullptr;

    juce::TextButton bypassBtn;
    juce::TextButton routingBtn;

    int dragHoverMode = 0;
    const juce::String dragID = "EFFECT_DEVICE_SLOT";
    void updateRoutingButtonText(); // Helper para actualizar la UI

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectDevice)
};