#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>

// ==============================================================================
// OfflineRenderer
// Ventana emergente estilo Reaper. Exporta el audio en un hilo de fondo (Background Thread)
// y dibuja la onda destructiva en tiempo real, marcando en ROJO los clips (> 0dBFS).
// ==============================================================================
class OfflineRenderer : public juce::Component, 
                        public juce::Thread, 
                        public juce::Timer
{
public:
    OfflineRenderer();
    ~OfflineRenderer() override;

    // --- EL PUENTE VITAL HACIA TU MOTOR DE AUDIO ---
    // Tu ProjectManager/AudioEngine debe suscribirse a este callback. 
    // Cuando el hilo lo llame, tu motor debe rellenar el buffer ignorando la tarjeta de sonido.
    std::function<void(juce::AudioBuffer<float>& buffer, int numSamples)> onProcessOfflineBlock;

    // Inicia el proceso de exportación
    void startRender(const juce::File& outputFile, double sampleRate, int bitDepth, double totalLengthSecs);
    
    // Detiene la exportación a la mitad si el usuario cancela
    void cancelRender();

    // Hilo de renderizado (Background)
    void run() override;

    // Hilo de la UI (Actualiza el dibujo a 30fps)
    void timerCallback() override;
    
    // Dibuja la onda y los marcadores de clipping
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::File targetFile;
    double targetSampleRate = 44100.0;
    int targetBitDepth = 24;
    double totalLengthSeconds = 0.0;
    
    juce::int64 totalSamplesToRender = 0;
    std::atomic<juce::int64> samplesRendered { 0 };

    std::atomic<bool> isFinished { false };
    std::atomic<bool> isCancelled { false };

    // --- Datos Analíticos y Visuales ---
    juce::CriticalSection dataLock; // Mutex ligero para pasar datos del Thread a la UI
    
    std::vector<float> waveMaxBuffer;
    std::vector<float> waveMinBuffer;
    std::vector<double> clipPositions; // Segundos exactos donde el audio superó 0 dBFS
    
    int totalClips = 0;
    float currentTruePeak = -100.0f;
    float currentLufsI = -100.0f;

    // UI Components
    juce::TextButton btnCancel;
    juce::Label statusLabel;
    juce::Label statsLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OfflineRenderer)
};