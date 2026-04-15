#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "../../../Engine/Modulation/NativeVisualSync.h"

/**
 * GainStationData: Estructura de datos para el módulo GainStation.
 * Sigue el patrón de "Caja Negra" aislada del Track.
 */
struct GainStationData {
    std::atomic<float> preGain { 1.0f };   // Input Trim
    std::atomic<float> postGain { 1.0f };  // Output Compensation
    std::atomic<bool>  isPhaseInverted { false };
    std::atomic<bool>  isMonoActive { false };

    // --- MODULACIÓN ---
    juce::LinearSmoothedValue<float> modPreGainSmoother;
    juce::LinearSmoothedValue<float> modPostGainSmoother;

    // --- MODULACIÓN VISUAL (Aislada) ---
    NativeVisualSync visSync;

    struct Snapshot {
        float preGain;
        float postGain;
        bool isPhaseInverted;
        bool isMonoActive;
    };

    Snapshot getSnapshot() const {
        return {
            preGain.load(std::memory_order_relaxed),
            postGain.load(std::memory_order_relaxed),
            isPhaseInverted.load(std::memory_order_relaxed),
            isMonoActive.load(std::memory_order_relaxed)
        };
    }

    GainStationData() {
        modPreGainSmoother.reset(44100.0, 0.02);
        modPostGainSmoother.reset(44100.0, 0.02);
    }

    void prepare(double sampleRate) {
        modPreGainSmoother.reset(sampleRate, 0.02);
        modPostGainSmoother.reset(sampleRate, 0.02);
    }
};
