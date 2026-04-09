#pragma once
#include <JuceHeader.h>
#include <atomic>

/**
 * GainStationData — Encapsula el estado de procesamiento del GainStation.
 * Sigue el patrón de "Caja Negra" aislada del Track.
 */
struct GainStationData {
    std::atomic<float> preGain { 1.0f };   // Input Trim
    std::atomic<float> postGain { 1.0f };  // Output Compensation
    std::atomic<bool>  isPhaseInverted { false };
    std::atomic<bool>  isMonoActive { false };

    // Snapshots para el hilo de audio (Copia atómica rápida)
    struct Snapshot {
        float preGain;
        float postGain;
        bool  isPhaseInverted;
        bool  isMonoActive;
    };

    Snapshot getSnapshot() const {
        return { 
            preGain.load(std::memory_order_relaxed),
            postGain.load(std::memory_order_relaxed),
            isPhaseInverted.load(std::memory_order_relaxed),
            isMonoActive.load(std::memory_order_relaxed)
        };
    }
};
