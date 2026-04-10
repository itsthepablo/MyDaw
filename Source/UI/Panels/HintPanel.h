#pragma once
#include <JuceHeader.h>

// ==============================================================================
// HINT PANEL (STATUS BAR INFERIOR)
// Muestra descripciones largas ocupando todo el ancho de la base del DAW
// ==============================================================================
class HintPanel : public juce::Component, private juce::Timer {
public:
    HintPanel();
    ~HintPanel() override = default;

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    juce::String currentHint;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HintPanel)
};