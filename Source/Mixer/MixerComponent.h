#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include "MixerChannelUI.h" // Incluimos el nuevo archivo

// ==============================================================================
// CONTENEDOR PRINCIPAL DEL MIXER 
// ==============================================================================
class MixerComponent : public juce::Component, private juce::Timer {
public:
    MixerComponent();
    ~MixerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateChannels();
    void timerCallback() override;

    void setTracksReference(const juce::OwnedArray<Track>* tracks) {
        tracksRef = tracks;
        updateChannels();
    }

    float getMasterVolume() const { return masterVolume; }
    void setMasterVolume(float v) { masterVolume = v; }

private:
    const juce::OwnedArray<Track>* tracksRef = nullptr;
    float masterVolume = 1.0f;

    juce::Viewport viewport;
    juce::Component contentComp;
    juce::OwnedArray<MixerChannelUI> channels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};