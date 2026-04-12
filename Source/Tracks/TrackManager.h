#pragma once
#include <JuceHeader.h>
#include "../UI/UIManager.h"
#include "../Engine/Core/AudioEngine.h"

class TrackManager {
public:
    TrackManager(DAWUIComponents& uiComponents, AudioEngine& engine, juce::CriticalSection& mutex);

    void setup();
    
    // Preparación técnica de audio
    void prepareTracks(double sampleRate, int samplesPerBlock);
    void prepareSingleTrack(Track* track, double sampleRate, int samplesPerBlock);

    // Handlers de eventos de UI
    void handleToggleAnalysisTrack(TrackType type, bool visible);
    void handleDeleteTrack(int index, std::function<void()> onPianoRollCleanup);
    void handleTrackAdded(Track* newTrack);
    void handleTracksReordered();

private:
    DAWUIComponents& ui;
    AudioEngine& audioEngine;
    juce::CriticalSection& audioMutex;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackManager)
};
