#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include "PeakMeter.h"
#include "../UI/Knobs/FLKnobLookAndFeel.h" // NUEVO: Importamos el estilo

class MixerStrip : public juce::Component {
public:
    MixerStrip(Track& t);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void syncWithTrack();
    Track& track;
    int lastDepth = -1;
    bool lastVis = true;
    juce::Colour lastColor;
private:
    juce::Slider volSlider, panSlider;
    juce::Label nameLabel;
    PeakMeter meterL, meterR;
    
    FLKnobLookAndFeel flLookAndFeel; // NUEVO: Instancia para la pista del mixer

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerStrip)
};

class MixerComponent : public juce::Component, private juce::Timer {
public:
    MixerComponent();
    ~MixerComponent() override;
    void setTracksReference(const juce::OwnedArray<Track>* tracks);
    float getMasterVolume() const;
    void updateStrips();
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
private:
    const juce::OwnedArray<Track>* tracksRef = nullptr;
    juce::OwnedArray<MixerStrip> strips;
    juce::Slider masterVol;
    juce::Label masterLabel;
    juce::Viewport viewport;
    juce::Component stripContainer;
    
    FLKnobLookAndFeel flLookAndFeel; // NUEVO: Instancia para el master del mixer

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};