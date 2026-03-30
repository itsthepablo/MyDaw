#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include "../../Engine/SimpleLoudness.h"

// ==============================================================================
// OfflineRenderer
// Ventana de Exportación Estilo Reaper (Configuración + Renderizado Destructivo)
// ==============================================================================
class OfflineRenderer : public juce::Component,
    public juce::Thread,
    public juce::Timer
{
public:
    OfflineRenderer();
    ~OfflineRenderer() override;

    std::function<void(juce::AudioBuffer<float>& buffer, int numSamples)> onProcessOfflineBlock;
    std::function<void()> onClose;

    // Abre la ventana en modo "Configuración"
    void showConfig(double totalLengthSecs);

    void startRender(const juce::File& outputFile, double sampleRate, int bitDepth, bool normalize);
    void cancelRender();

    void run() override;
    void timerCallback() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    enum class ViewState { Configuration, Rendering };
    ViewState currentState = ViewState::Configuration;

    // --- ELEMENTOS DE CONFIGURACIÓN (Estilo Reaper) ---
    juce::Label lblTitle{ "", "Render to File" };
    juce::Label lblSampleRate{ "", "Sample rate:" };
    juce::ComboBox cbSampleRate;
    juce::Label lblBitDepth{ "", "WAV bit depth:" };
    juce::ComboBox cbBitDepth;
    juce::ToggleButton cbNormalize{ "Normalize to 0.0 dB" };
    juce::TextButton btnStart{ "Render 1 file..." };
    juce::TextButton btnCloseConfig{ "Cancel" };
    std::unique_ptr<juce::FileChooser> fileChooser;

    // --- ELEMENTOS DE RENDERIZADO ---
    juce::File targetFile;
    double targetSampleRate = 44100.0;
    int targetBitDepth = 24;
    double totalLengthSeconds = 0.0;

    juce::int64 totalSamplesToRender = 0;
    std::atomic<juce::int64> samplesRendered{ 0 };

    std::atomic<bool> isFinished{ false };
    std::atomic<bool> isCancelled{ false };
    bool shouldNormalize = false;
    bool isNormalizing = false;

    juce::CriticalSection dataLock;

    std::vector<float> waveMaxBuffer;
    std::vector<float> waveMinBuffer;
    std::vector<double> clipPositions;

    int totalClips = 0;
    float currentTruePeak = -100.0f;
    
    // --- LOGICA REAPER ---
    SimpleLoudness loudnessMeter;
    float currentLufsM = -100.0f;
    float currentLufsS = -100.0f;
    float currentLufsI = -100.0f;
    float currentLra = 0.0f;

    juce::TextButton btnCancelRender;
    juce::TextButton btnOpenFolder{ "Open folder..." };
    juce::TextButton btnLaunchFile{ "Launch file" };

    juce::Label statusLabel;
    juce::Label statsLabel;
    juce::Label timeLabel;

    void updateVisibility();
    void performNormalization();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OfflineRenderer)
};