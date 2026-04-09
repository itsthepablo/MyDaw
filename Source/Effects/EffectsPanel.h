#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include <functional>
#include <map> 
#include "EffectDevice.h"
#include "SendDevice.h"
#include "../Modules/GainStation/UI/GainStationPanel.h"

// ==============================================================================
// EFFECTS PANEL (Contenedor Principal de Plugins y Envíos)
// ==============================================================================
class EffectsPanel : public juce::Component {
public:
    EffectsPanel();

    void setTrack(Track* t);
    void updateSlots();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateStyles();
    void lookAndFeelChanged() override { updateStyles(); repaint(); }

    Track* getActiveTrack() const { return activeTrack; }

    std::function<void(Track&)> onAddVST3;           
    std::function<void(Track&, const juce::String&, const juce::String&, int)> onAddVST3FromFile; 
    std::function<void(Track&)> onAddNativeUtility;  
    std::function<void(Track&)> onAddNativeOrion;
    std::function<void(Track&, int)> onOpenEffect;
    std::function<void(Track&, int, bool)> onBypassChanged;
    std::function<void(Track&, int, int)> onReorderEffects;
    std::function<void(Track&, int)> onDeleteEffect;
    
    // --- AUDIO SENDS ---
    std::function<void(Track&)> onAddSend;
    std::function<void(Track&, int)> onDeleteSend;
    std::function<void(Track&)> onChangeSend; 

    std::function<juce::Array<Track*>()> getAvailableTracks; 

private:
    Track* activeTrack = nullptr;

    juce::TextButton bypassAllBtn;
    juce::TextButton addEffectBtn;
    juce::TextButton addSendBtn;

    juce::OwnedArray<EffectDevice> devices;
    juce::OwnedArray<SendDevice> sends;

    bool isGainStationExpanded = true;
    juce::TextButton toggleGainStationBtn; 
    juce::TextButton hideGainStationBtn;   

    GainStationPanel gainStation;
    std::unique_ptr<GainStationBridge> gsBridge;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};