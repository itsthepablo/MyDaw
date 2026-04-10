#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include <functional>
#include "../Engine/SimpleLoudness.h"
// ==============================================================================
// RenderEngine
// Motor de renderizado destructivo offline con métricas EBU R128 y True Peak.
// No hereda de juce::Component. Se ejecuta puro y asíncrono.
// ==============================================================================
class RenderEngine : public juce::Thread
{
public:
    RenderEngine();
    ~RenderEngine() override;

    // --- Inyección de bloques de audio desde el framework externo ---
    std::function<void(juce::AudioBuffer<float>& buffer, int numSamples)> onProcessOfflineBlock;
    std::function<void(double sampleRate)> onPrepareEngine;
    std::function<void(bool success)> onRenderFinished;

    // --- Control de Exportación ---
    void startRender(const juce::File& outputFile, double sampleRate, int bitDepth, bool normalize, double totalLengthSecs);
    void cancelRender();

    // Hilo de proceso (DSP y Disco)
    void run() override;

    // --- Estados para UI (Lock-Free Atomics) ---
    enum class EngineState { Idle, Rendering, Normalizing, Finished, Cancelled, Error };
    std::atomic<EngineState> currentState{ EngineState::Idle };

    // --- Lecturas Atómicas para Timer Visual (Lock-Free) ---
    std::atomic<int64_t> samplesRendered{ 0 };
    std::atomic<int64_t> totalSamplesToRender{ 0 };
    std::atomic<int> totalClips{ 0 };

    float getCurrentLufsM() const { return currentLufsM.load(std::memory_order_relaxed); }
    float getCurrentLufsS() const { return currentLufsS.load(std::memory_order_relaxed); }
    float getCurrentLufsI() const { return currentLufsI.load(std::memory_order_relaxed); }
    float getCurrentLra() const   { return currentLra.load(std::memory_order_relaxed); }
    float getCurrentTruePeak() const { return currentTruePeak.load(std::memory_order_relaxed); }
    
    juce::String getErrorMessage() const { return errorMessage; }

    // --- Datos compartidos para trazar la onda en la UI ---
    // Utilizamos un mutex muy leve exclusivo para la copia del osciloscopio
    void getWaveformSnapshot(std::vector<float>& maxBuf, std::vector<float>& minBuf);

private:
    void performNormalization();

    juce::File targetFile;
    double targetSampleRate = 44100.0;
    int targetBitDepth = 24;
    double totalLengthSeconds = 0.0;
    bool shouldNormalize = false;

    // --- MÓDULOS DE ANÁLISIS ---
    SimpleLoudness loudnessMeter;

    std::atomic<float> currentLufsM{ -100.0f };
    std::atomic<float> currentLufsS{ -100.0f };
    std::atomic<float> currentLufsI{ -100.0f };
    std::atomic<float> currentLra{ 0.0f };
    std::atomic<float> currentTruePeak{ 0.0f };

    juce::String errorMessage;

    juce::CriticalSection waveDataLock;
    std::vector<float> waveMaxBuffer;
    std::vector<float> waveMinBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RenderEngine)
};
