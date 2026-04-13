#pragma once
#include <JuceHeader.h>
#include "../../Tracks/Data/TrackData.h"
#include "../../Data/SmartColorUtils.h"
#include "AudioClipStyles.h"
#include <vector>
#include <atomic>

/**
 * AudioPeak: Par de picos para renderizado.
 */
struct AudioPeak {
    float maxPos = 0.0f;
    float minNeg = 0.0f;
};

/**
 * AudioClip: "Caja Negra" que gestiona los datos de un clip de audio.
 * Encapsula la carga de archivos, generación de picos y estados internos.
 */
class AudioClip {
public:
    AudioClip();
    ~AudioClip() = default;

    // --- COPIA DE BLOQUEO (PARA SNAPSHOTS) ---
    AudioClip(const AudioClip& other);
    AudioClip& operator=(const AudioClip& other);

    // --- LÓGICA DE CARGA ---
    bool loadFromFile(const juce::File& file, double targetSampleRate);

    // --- GETTERS / SETTERS ---
    juce::String getName() const { return name; }
    void setName(const juce::String& n);

    juce::Colour getColor() const { return color; }
    void setColor(juce::Colour c, bool isManualAssignment = false) { 
        color = c; 
        if (isManualAssignment) isColorManual = true; 
    }

    float getStartX() const { return startX; }
    void setStartX(float x) { startX = x; }

    float getWidth() const { return width; }
    void setWidth(float w) { width = w; }

    float getOffsetX() const { return offsetX; }
    void setOffsetX(float o) { offsetX = o; }

    float getOriginalWidth() const { return originalWidth; }
    
    bool getIsMuted() const { return isMuted; }
    void setIsMuted(bool m) { isMuted = m; }

    bool getIsSelected() const { return isSelected; }
    void setIsSelected(bool s) { isSelected = s; }

    bool isLoaded() const { return isLoadedFlag.load(std::memory_order_relaxed); }

    WaveformStyle getStyle() const { return style; }
    void setStyle(WaveformStyle s) { style = s; }

    juce::AudioBuffer<float>& getBuffer() { return fileBuffer; }
    const juce::AudioBuffer<float>& getBuffer() const { return fileBuffer; }

    juce::Image& getSpectrogram() { return spectrogramImage; }
    juce::AudioThumbnail* getThumbnail() { return thumbnail.get(); }
    void setThumbnail(std::unique_ptr<juce::AudioThumbnail> t) { thumbnail = std::move(t); }
    juce::String getSourceFilePath() const { return sourceFilePath; }
    
    double getSpectrogramHopSize() const { return spectrogramHopSize; }

    void generateCache();
    
    // --- NUEVO: GESTION DIFERIDA DEL ESPECTROGRAMA ---
    void ensureSpectrogramIsGenerated();
    bool isSpectrogramValid() const { return spectrogramGenerated.load(std::memory_order_relaxed); }
    bool isSpectrogramLoading() const { return spectrogramInProgress.load(std::memory_order_relaxed); }
    
    // --- DEBUG / DIAGNÓSTICO ---
    static bool isDebugInfoEnabled() { return showWaveformDebugInfo; }
    static void setDebugInfoEnabled(bool enabled) { showWaveformDebugInfo = enabled; }

    // --- DATOS DE PICOS (MIP-MAPS) ---
    const std::vector<AudioPeak>& getPeaksL(int lod) const;
    const std::vector<AudioPeak>& getPeaksR(int lod) const;
    const std::vector<AudioPeak>& getPeaksMid(int lod) const;
    const std::vector<AudioPeak>& getPeaksSide(int lod) const;

private:
    juce::String name;
    float startX = 0.0f;
    float width = 0.0f;
    float offsetX = 0.0f;
    float originalWidth = 0.0f;
    bool isMuted = false;
    bool isSelected = false;
    juce::Colour color;
    bool isColorManual = false;
    WaveformStyle style = WaveformStyle::NoBackground;
    std::atomic<bool> isLoadedFlag{ false };
    static bool showWaveformDebugInfo;
    
    juce::String sourceFilePath;
    juce::AudioBuffer<float> fileBuffer;
    double sourceSampleRate = 44100.0;

    // Mip-Maps
    std::vector<std::vector<AudioPeak>> peaksL, peaksR, peaksMid, peaksSide; // [lod][pixel]

    juce::Image spectrogramImage;
    double spectrogramHopSize = 64.0;
    std::atomic<bool> spectrogramGenerated{ false };
    std::atomic<bool> spectrogramInProgress{ false };
    std::unique_ptr<juce::AudioThumbnail> thumbnail;

    // Cache Helpers
    void generateSpectrogram();
    static std::vector<AudioPeak> buildLOD(const std::vector<AudioPeak>& source, int factor);

    JUCE_LEAK_DETECTOR(AudioClip)
};
