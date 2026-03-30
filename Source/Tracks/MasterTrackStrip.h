#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include "../UI/LevelMeter.h"

// ==============================================================================
// MasterTrackStrip — Barra de Master fija (como Ableton/Bitwig)
//
// Siempre visible en la parte inferior de la vista Arrangement.
// Permite insertar plugins en el bus maestro y ver el nivel de salida.
// ==============================================================================
class MasterTrackStrip : public juce::Component,
                         public juce::Timer
{
public:
    // Callbacks hacia MainComponent
    std::function<void()> onOpenMasterFx;

    MasterTrackStrip();
    ~MasterTrackStrip() override;

    void setMasterTrack(Track* t);
    Track* getMasterTrack() const;

    void timerCallback() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    Track* masterTrack = nullptr;

    juce::Label      masterLabel;
    juce::TextButton fxButton;
    juce::Slider     volumeSlider;
    LevelMeter       levelMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterTrackStrip)
};