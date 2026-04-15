#pragma once
#include <JuceHeader.h>
#include <atomic>

/**
 * NativeVisualSync: Bus de datos optimizado para la visualización de modulación.
 * Ahora incluye suavizado visual para evitar el efecto de "doble imagen" en faders rápidos.
 */
struct NativeVisualSync {
    // Mixer
    std::atomic<float> vol { 1.0f }, pan { 0.0f };
    std::atomic<bool> hasVol { false }, hasPan { false };

    // GainStation
    std::atomic<float> pre { 1.0f }, post { 1.0f };
    std::atomic<bool> hasPre { false }, hasPost { false };

    // EQ
    std::atomic<float> b1Freq { 150.0f }, b1Gain { 0.0f }, b1Q { 0.707f };
    std::atomic<bool> hasB1Freq { false }, hasB1Gain { false }, hasB1Q { false };
    
    std::atomic<float> b2Freq { 5000.0f }, b2Gain { 0.0f }, b2Q { 0.707f };
    std::atomic<bool> hasB2Freq { false }, hasB2Gain { false }, hasB2Q { false };

    /**
     * Helper para suavizado visual (Interpolación Lineal).
     * Evita que el slider salte bruscamente entre frames de la UI.
     */
    static float smooth(float current, float target, float factor = 0.2f) {
        if (!std::isfinite(target)) return current;
        return current + factor * (target - current);
    }
};
