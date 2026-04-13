#pragma once
#include <JuceHeader.h>
#include <atomic>

struct TransportState {
    std::atomic<bool> isPlaying { false };
    std::atomic<bool> isStoppingNow { false };
    std::atomic<bool> isPreviewing { false };
    std::atomic<int> previewPitch { -1 };
    
    std::atomic<float> bpm { 120.0f };
    std::atomic<float> loopEndPos { 32.0f * 320.0f };
    std::atomic<float> playheadPos { 0.0f };
    std::atomic<float> seekRequestPh { -1.0f };
    
    // Comunicación de vuelta desde Audio -> UI (Lock-Free)
    std::atomic<float> currentAudioPlayhead { 0.0f };
    std::atomic<int> currentGlobalLatency { 0 };
    std::atomic<double> lastPlayheadUpdateMs { 0.0 };
};
