#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"

struct TrackClip {
    Track* trackPtr;
    float startX;
    float width;
    juce::String name;
    AudioClipData* linkedAudio = nullptr;
    MidiClipData* linkedMidi = nullptr;
};
