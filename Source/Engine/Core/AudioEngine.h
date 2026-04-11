#pragma once
#include <JuceHeader.h>
#include "AudioClock.h"
#include "TransportState.h"
#include "../Routing/RoutingMatrix.h"
#include "../Nodes/MetronomeProcessor.h"
#include "../Threading/AudioThreadPool.h"

// Forward declarations
class Track;

/**
 * AudioEngine: Motor central de procesamiento de audio multihilo.
 * Implementa una arquitectura de tres fases para el procesamiento paralelo.
 */
class AudioEngine {
public:
    AudioClock clock;
    MetronomeProcessor metronome;
    TransportState transportState;
    RoutingMatrix routingMatrix;
    AudioThreadPool threadPool;

    double currentSampleRate = 44100.0;
    Track* masterTrack = nullptr;

    void prepareToPlay(int samples, double s);
    void releaseResources();
    void resetForRender();
    void setNonRealtime(bool isNonRealtime);
    void setMasterTrack(Track* t) { masterTrack = t; }

    std::atomic<Track*> selectedTrackForAnalysis { nullptr };
    std::atomic<void*> analyzerToFeed { nullptr }; // Cast to SimpleAnalyzer* in cpp
    std::atomic<void*> vuToFeed { nullptr };       // Cast to VUBallistics* in cpp

    void processBlock(const juce::AudioSourceChannelInfo& bufferToFill) noexcept;
};