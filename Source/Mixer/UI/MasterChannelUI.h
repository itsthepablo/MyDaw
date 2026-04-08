#pragma once
#include <JuceHeader.h>
#include <functional>
#include <memory>
#include "MixerChannelUI.h"
#include "../../UI/LevelMeter.h"

// ==============================================================================
// MASTER CHANNEL UI
// Declaración — La implementación está en MasterChannelUI.cpp
// ==============================================================================
class MasterChannelUI : public juce::Component {
public:
    MasterChannelUI(std::function<float()> getVol, std::function<void(float)> setVol);
    ~MasterChannelUI() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::function<float()> getVolumeCallback;
    std::function<void(float)> setVolumeCallback;

    MixerLookAndFeel mixerLAF;
    juce::Slider volumeSlider;
    juce::Label trackName;
    
    // Cambiado de AdvancedMeter (inexistente) a LevelMeter para consistencia
    std::unique_ptr<LevelMeter> meter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterChannelUI)
};
