#pragma once
#include <JuceHeader.h>
#include "../../Tracks/Track.h"

// ==============================================================================
// SLOTS Y RACKS (Componentes Auxiliares Aislados)
// Declaraciones — La implementación está en MixerRacks.cpp
// ==============================================================================

class PluginSlot : public juce::Component {
public:
    PluginSlot(Track* t, int idx);
    void syncWithModel();
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    std::function<void(int)> onOpenPlugin, onDeletePlugin, onAddNativeUtility, onAddVST3;
    std::function<void(int, bool)> onBypassChanged;

private:
    Track* track; 
    int index; 
    juce::TextButton bypassBtn;
};


class SendSlot : public juce::Component {
public:
    SendSlot(Track* t, int idx);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    
    std::function<void()> onAddSend; 
    std::function<void(int)> onDeleteSend;

private:
    Track* track; 
    int index;
};