#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include <functional>
#include <map> 

class EffectsPanel;

// --- COMPONENTE INDIVIDUAL PARA CADA EFECTO EN LA LISTA ---
class EffectSlot : public juce::Component, public juce::DragAndDropTarget {
public:
    EffectSlot(int index, juce::String name, bool isInst, EffectsPanel& p);

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const SourceDetails& dragSourceDetails) override;
    void itemDragExit(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    int idx;
    juce::String fxName;
    bool isInstrument;
    EffectsPanel& panel;

    bool isDragHovering = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectSlot)
};

// --- PANEL LATERAL DE EFECTOS ---
class EffectsPanel : public juce::Component {
public:
    EffectsPanel();

    void setTrack(Track* t);
    void updateSlots();

    void paint(juce::Graphics& g) override;
    void resized() override;

    Track* getActiveTrack() const { return activeTrack; }

    static std::map<void*, bool> pluginIsInstrumentMap;

    std::function<void()> onClose;
    std::function<void(Track&)> onAddEffect;
    std::function<void(Track&, int)> onOpenEffect;
    std::function<void(Track&, int, int)> onReorderEffects;

private:
    Track* activeTrack = nullptr;
    juce::TextButton closeBtn;
    juce::TextButton addEffectBtn;

    juce::OwnedArray<EffectSlot> slots;

    // NUEVO: Coordenadas físicas para separar la interfaz
    int instHeaderY = 0;
    int fxHeaderY = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};