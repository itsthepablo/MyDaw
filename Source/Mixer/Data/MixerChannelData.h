#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "../../Engine/Modulation/NativeVisualSync.h"

/**
 * MixerChannelData: Contenedor de datos y parámetros de mezcla.
 */
struct MixerChannelData {
    MixerChannelData() {
        // --- INICIALIZACIÓN CRÍTICA ---
        // Los smoothers deben empezar en 1.0f para que el audio no se silencie al arrancar.
        volumeSmoother.reset(44100.0, 0.05);
        volumeSmoother.setCurrentAndTargetValue(1.0f);
        
        panSmoother.reset(44100.0, 0.05);
        panSmoother.setCurrentAndTargetValue(0.0f);
        
        midVolumeSmoother.reset(44100.0, 0.05);
        midVolumeSmoother.setCurrentAndTargetValue(1.0f);
        
        sideVolumeSmoother.reset(44100.0, 0.05);
        sideVolumeSmoother.setCurrentAndTargetValue(1.0f);
        
        modVolumeSmoother.reset(44100.0, 0.02);
        modVolumeSmoother.setCurrentAndTargetValue(0.0f);
        
        modPanSmoother.reset(44100.0, 0.02);
        modPanSmoother.setCurrentAndTargetValue(0.0f);
    }

    // --- PARÁMETROS DE CONTROL (BASE) ---
    float volume = 1.0f;
    float balance = 0.0f;
    float midVolume = 1.0f;
    float sideVolume = 1.0f;

    // Smoothers
    juce::LinearSmoothedValue<float> volumeSmoother;
    juce::LinearSmoothedValue<float> panSmoother;
    juce::LinearSmoothedValue<float> midVolumeSmoother;
    juce::LinearSmoothedValue<float> sideVolumeSmoother;
    juce::LinearSmoothedValue<float> modVolumeSmoother;
    juce::LinearSmoothedValue<float> modPanSmoother;

    // --- ESTADOS ATÓMICOS ---
    std::atomic<bool> isMuted { false };
    std::atomic<bool> isSoloed { false };
    std::atomic<bool> isMidSolo { false };
    std::atomic<bool> isSideSolo { false };
    std::atomic<bool> isMidSideMode { false };
    
    // --- GESTIÓN DE PANEO ---
    std::atomic<bool> panningModeDual { false };
    std::atomic<float> panL { -1.0f };
    std::atomic<float> panR { 1.0f };

    // --- METERING & PEAKS ---
    std::atomic<float> currentPeakLevelL { 0.0f };
    std::atomic<float> currentPeakLevelR { 0.0f };
    std::atomic<float> currentMidPeak { 0.0f };
    std::atomic<float> currentSidePeak { 0.0f };

    // --- MODULACIÓN VISUAL (AISLADA) ---
    NativeVisualSync visSync;

    void prepare(double sampleRate, int /*samplesPerBlock*/) {
        volumeSmoother.reset(sampleRate, 0.05);
        panSmoother.reset(sampleRate, 0.05);
        midVolumeSmoother.reset(sampleRate, 0.05);
        sideVolumeSmoother.reset(sampleRate, 0.05);
        modVolumeSmoother.reset(sampleRate, 0.02);
        modPanSmoother.reset(sampleRate, 0.02);
    }

    void setVolume(float v) { volume = v; volumeSmoother.setTargetValue(v); }
    void setBalance(float b) { balance = b; panSmoother.setTargetValue(b); }
    void setMidVolume(float v) { midVolume = v; midVolumeSmoother.setTargetValue(v); }
    void setSideVolume(float v) { sideVolume = v; sideVolumeSmoother.setTargetValue(v); }
};
