#pragma once
#include <JuceHeader.h>
#include "../../../UI/Knobs/FloatingValueSlider.h"
#include "GainStationMeter.h"
#include "../Bridges/GainStationBridge.h"

/**
 * GainStationPanel — Interfaz principal del módulo GainStation.
 * Ofrece controles de Pre-Gain, Post-Gain, Fase y Mono Sum.
 */
class GainStationPanel : public juce::Component, public juce::Timer {
public:
    GainStationPanel();
    ~GainStationPanel() override;

    void setBridge(GainStationBridge* b);
    int getPreferredWidth() const { return 180; }
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    void performGainMatch();
    void timerCallback() override;

    GainStationBridge* activeBridge = nullptr;
    GainStationMeter meter;
    FloatingValueSlider preGainKnob, postGainKnob;
    juce::TextButton phaseBtn, monoBtn, matchBtn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainStationPanel)
};
