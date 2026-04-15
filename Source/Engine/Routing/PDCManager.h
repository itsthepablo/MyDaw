#pragma once
#include <JuceHeader.h>
#include "RoutingMatrix.h"
#include "../Core/TransportState.h"

class PDCManager {
public:
    static inline std::atomic<int> currentGlobalLatency { 0 };

    // ------ DIAGNÓSTICO DEL AUDIO THREAD (SOLO DEBUGGING EN VIVO) ------
    static inline std::atomic<int> dbgTracks { 0 };
    static inline std::atomic<int> dbgPlaying { 0 };
    static inline std::atomic<int> dbgClips { 0 };
    static inline std::atomic<int> dbgSamplesWritten { 0 };
    static inline std::atomic<int> dbgAddCount { 0 };
    static inline std::atomic<int> dbgMagCheck { 0 };
    // -------------------------------------------------------------------

    static void calculateLatencies(const RoutingMatrix::TopoState* state, TransportState& ts) noexcept;
    static void applyDelay(Track* track, int numSamples) noexcept;
};
