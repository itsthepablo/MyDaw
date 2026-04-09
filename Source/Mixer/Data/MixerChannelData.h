#pragma once
#include <JuceHeader.h>

/**
 * MixerChannelData: Contenedor de datos y parámetros de mezcla.
 * Esta clase encapsula el estado de un canal (volumen, pan, mute, solo, fase)
 * y garantiza la seguridad en tiempo real mediante el uso de smoothers y atómicos.
 */
struct MixerChannelData {
    MixerChannelData() {
        // Inicialización segura de faders para evitar ruidos iniciales
        volumeSmoother.reset(44100.0, 0.05);
        volumeSmoother.setCurrentAndTargetValue(volume);
        panSmoother.reset(44100.0, 0.05);
        panSmoother.setCurrentAndTargetValue(balance);
    }

    // --- PARÁMETROS DE CONTROL ---
    float volume = 1.0f;
    float balance = 0.0f;

    // Smoothers para evitar clicks/pops (Sigue la Física del Sonido - Regla #18)
    juce::LinearSmoothedValue<float> volumeSmoother;
    juce::LinearSmoothedValue<float> panSmoother;

    // --- ESTADOS ATÓMICOS (Thread-Safe) ---
    std::atomic<bool> isMuted { false };
    std::atomic<bool> isSoloed { false };
    
    // --- GESTIÓN DE PANEO ---
    std::atomic<bool> panningModeDual { false };
    std::atomic<float> panL { -1.0f };
    std::atomic<float> panR { 1.0f };

    // --- METERING (Lectura desde UI, Escritura desde AudioThread) ---
    std::atomic<float> currentPeakLevelL { 0.0f };
    std::atomic<float> currentPeakLevelR { 0.0f };

    /**
     * Prepara los smoothers para el procesamiento de audio.
     */
    void prepare(double sampleRate, int /*samplesPerBlock*/) {
        volumeSmoother.reset(sampleRate, 0.05);
        volumeSmoother.setCurrentAndTargetValue(volume);

        panSmoother.reset(sampleRate, 0.05);
        panSmoother.setCurrentAndTargetValue(balance);
    }

    void setVolume(float v) {
        volume = v;
        volumeSmoother.setTargetValue(v);
    }

    void setBalance(float b) {
        balance = b;
        panSmoother.setTargetValue(b);
    }
};
