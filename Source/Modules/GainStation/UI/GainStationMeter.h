#pragma once
#include <JuceHeader.h>
#include "../Bridges/GainStationBridge.h"

/**
 * GainStationMeter — Componente visual para los medidores de entrada/salida del GainStation.
 */
class GainStationMeter : public juce::Component, public juce::Timer {
public:
    GainStationMeter();
    ~GainStationMeter() override;

    void setBridge(GainStationBridge* b);
    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    GainStationBridge* activeBridge = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainStationMeter)
};
