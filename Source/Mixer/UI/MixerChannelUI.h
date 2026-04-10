#pragma once
#include <JuceHeader.h>
#include "../../Data/Track.h"
#include "../Bridges/MixerParameterBridge.h"
#include "../../UI/Meters/LevelMeter.h"
#include "../LookAndFeel/MixerLookAndFeel.h"
#include "MixerRacks.h"

// ==============================================================================
// 4. CANAL PRINCIPAL DEL MIXER (Racks y Faders)
// Declaración — La implementación está en MixerChannelUI.cpp
// ==============================================================================
class MixerChannelUI : public juce::Component {
public:
    // --- Sub-componentes para Rack de Efectos ---
    class FXRack : public juce::Component {
    public:
        FXRack(Track* t, MixerChannelUI* p);
        void syncSlots();
        void resized() override;
    private:
        Track* track; MixerChannelUI* parent; juce::OwnedArray<PluginSlot> slots;
    };

    // --- Sub-componentes para Rack de Envios ---
    class SendRack : public juce::Component {
    public:
        SendRack(Track* t, MixerChannelUI* p);
        void syncSlots();
        void resized() override;
    private:
        Track* track; MixerChannelUI* parent; juce::OwnedArray<SendSlot> slots;
    };

    MixerChannelUI(Track* t, bool miniMode = false);
    ~MixerChannelUI() override;

    void updateUI();
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    // Callbacks del sistema
    std::function<void(Track&)> onAddVST3, onAddNativeUtility, onAddSend;
    std::function<void(Track&, int)> onOpenPlugin, onDeleteEffect, onDeleteSend;
    std::function<void(Track&, int, bool)> onBypassChanged;
    std::function<void(Track&, int, juce::String)> onCreateAutomation;

private:
    void showAutomationMenu(int paramId, const juce::String& paramName);
    void setupButton(juce::TextButton& btn, juce::String text, juce::Colour onCol);
    void updatePanVisibility();

    bool isMiniMode; 
    Track* track; 
    MixerLookAndFeel mixerLAF;
    
    juce::TextButton panToggle;
    juce::Slider panKnob, panL, panR, fader; 
    LevelMeter meter;
    juce::TextButton muteBtn, soloBtn, phaseBtn, recBtn; 
    juce::Label trackName;
    
    juce::Viewport fxViewport, sendViewport;
    FXRack fxRack;
    SendRack sendRack;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelUI)
};
