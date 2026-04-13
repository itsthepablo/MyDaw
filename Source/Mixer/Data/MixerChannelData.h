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

        midVolumeSmoother.reset(44100.0, 0.05);
        midVolumeSmoother.setCurrentAndTargetValue(midVolume);
        sideVolumeSmoother.reset(44100.0, 0.05);
        sideVolumeSmoother.setCurrentAndTargetValue(sideVolume);

        modVolumeSmoother.reset(44100.0, 0.02); // Suavizado más rápido para LFOs
        modVolumeSmoother.setCurrentAndTargetValue(0.0f);
        modPanSmoother.reset(44100.0, 0.02);
        modPanSmoother.setCurrentAndTargetValue(0.0f);
    }

    // --- PARÁMETROS DE CONTROL ---
    float volume = 1.0f;
    float balance = 0.0f;
    float midVolume = 1.0f;
    float sideVolume = 1.0f;

    // Smoothers para evitar clicks/pops (Sigue la Física del Sonido - Regla #18)
    juce::LinearSmoothedValue<float> volumeSmoother;
    juce::LinearSmoothedValue<float> panSmoother;
    juce::LinearSmoothedValue<float> midVolumeSmoother;
    juce::LinearSmoothedValue<float> sideVolumeSmoother;
    juce::LinearSmoothedValue<float> modVolumeSmoother;
    juce::LinearSmoothedValue<float> modPanSmoother;

    // --- ESTADOS ATÓMICOS (Thread-Safe) ---
    std::atomic<bool> isMuted { false };
    std::atomic<bool> isSoloed { false };
    std::atomic<bool> isMidSolo { false };
    std::atomic<bool> isSideSolo { false };
    std::atomic<bool> isMidSideMode { false };
    
    // --- GESTIÓN DE PANEO ---
    std::atomic<bool> panningModeDual { false };
    std::atomic<float> panL { -1.0f };
    std::atomic<float> panR { 1.0f };

    // --- METERING (Lectura desde UI, Escritura desde AudioThread) ---
    std::atomic<float> currentPeakLevelL { 0.0f };
    std::atomic<float> currentPeakLevelR { 0.0f };
    std::atomic<float> currentMidPeak { 0.0f };
    std::atomic<float> currentSidePeak { 0.0f };

    /**
     * Prepara los smoothers para el procesamiento de audio.
     */
    void prepare(double sampleRate, int /*samplesPerBlock*/) {
        volumeSmoother.reset(sampleRate, 0.05);
        volumeSmoother.setCurrentAndTargetValue(volume);

        panSmoother.reset(sampleRate, 0.05);
        panSmoother.setCurrentAndTargetValue(balance);

        midVolumeSmoother.reset(sampleRate, 0.05);
        midVolumeSmoother.setCurrentAndTargetValue(midVolume);

        sideVolumeSmoother.reset(sampleRate, 0.05);
        sideVolumeSmoother.setCurrentAndTargetValue(sideVolume);

        modVolumeSmoother.reset(sampleRate, 0.02);
        modPanSmoother.reset(sampleRate, 0.02);
    }

    void setVolume(float v) {
        volume = v;
        volumeSmoother.setTargetValue(v);
    }

    void setBalance(float b) {
        balance = b;
        panSmoother.setTargetValue(b);
    }

    void setMidVolume(float v) {
        midVolume = v;
        midVolumeSmoother.setTargetValue(v);
    }

    void setSideVolume(float v) {
        sideVolume = v;
        sideVolumeSmoother.setTargetValue(v);
    }
};
