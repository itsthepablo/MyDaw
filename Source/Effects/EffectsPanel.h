#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include <functional>
#include <map> 
#include "EffectDevice.h"
#include "../UI/GainStation/GainStationPanel.h" // Inclusión del nuevo módulo

// ==============================================================================
// EFFECTS PANEL (Contenedor Principal)
// ==============================================================================
class EffectsPanel : public juce::Component {
public:
    EffectsPanel();

    void setTrack(Track* t);
    void updateSlots();

    void paint(juce::Graphics& g) override;
    void resized() override;

    Track* getActiveTrack() const { return activeTrack; }

    std::function<void(Track&)> onAddVST3;           // <-- Para cargar VST3 normal
    std::function<void(Track&, const juce::String&, const juce::String&, int)> onAddVST3FromFile; // <-- NUEVO: Para cargar desde archivo (Proyecto)
    std::function<void(Track&)> onAddNativeUtility;  // <-- NUEVO: Carga el Utility nativo
    std::function<void(Track&, int)> onOpenEffect;
    std::function<void(Track&, int, bool)> onBypassChanged;
    std::function<void(Track&, int, int)> onReorderEffects;
    std::function<void(Track&, int)> onDeleteEffect;
    std::function<juce::Array<Track*>()> getAvailableTracks; 

private:
    Track* activeTrack = nullptr;

    juce::TextButton bypassAllBtn;
    juce::TextButton addEffectBtn;
    juce::OwnedArray<EffectDevice> devices;

    // Estado y botones de control del panel izquierdo
    bool isGainStationExpanded = true;
    juce::TextButton toggleGainStationBtn; 
    juce::TextButton hideGainStationBtn;   

    // El contenedor es la Gain Station. 
    GainStationPanel loudnessMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};