#pragma once
#include <JuceHeader.h>
#include "../GlobalData.h" 
#include "../PluginHost/VSTHost.h" 
#include <vector>
#include <algorithm>
#include <cmath>

enum class TrackType { MIDI, Audio };
enum class WaveformViewMode { Combined, SeparateLR, MidSide }; // NUEVO: Modo Mid/Side

struct AudioClipData {
    juce::String name;
    float startX = 0.0f;
    float width = 0.0f;
    juce::AudioBuffer<float> fileBuffer;
    double sourceSampleRate = 44100.0;

    // --- CACHÉ MULTIVISTA ---
    std::vector<float> cachedPeaksL;
    std::vector<float> cachedPeaksR;
    std::vector<float> cachedPeaksMid;  // Caché para (L+R)/2
    std::vector<float> cachedPeaksSide; // Caché para (L-R)/2

    void generateCache() {
        const int samplesPerPoint = 256;
        const int numSamples = fileBuffer.getNumSamples();
        if (numSamples <= 0) return;

        const int numPoints = numSamples / samplesPerPoint;

        cachedPeaksL.clear(); cachedPeaksL.reserve(numPoints);
        cachedPeaksR.clear(); cachedPeaksMid.clear(); cachedPeaksSide.clear();

        bool isStereo = fileBuffer.getNumChannels() > 1;
        if (isStereo) {
            cachedPeaksR.reserve(numPoints);
            cachedPeaksMid.reserve(numPoints);
            cachedPeaksSide.reserve(numPoints);
        }

        // LECTURA OPTIMIZADA: Escaneamos L, R, Mid y Side en una sola pasada de CPU
        for (int i = 0; i < numPoints; ++i) {
            int startSample = i * samplesPerPoint;
            int numSamplesInPoint = std::min(samplesPerPoint, numSamples - startSample);

            float pL = 0.0f, pR = 0.0f, pMid = 0.0f, pSide = 0.0f;

            if (isStereo) {
                const float* lData = fileBuffer.getReadPointer(0, startSample);
                const float* rData = fileBuffer.getReadPointer(1, startSample);

                for (int s = 0; s < numSamplesInPoint; ++s) {
                    float l = lData[s];
                    float r = rData[s];

                    pL = std::max(pL, std::abs(l));
                    pR = std::max(pR, std::abs(r));

                    // Matemática Mid/Side
                    pMid = std::max(pMid, std::abs((l + r) * 0.5f));
                    pSide = std::max(pSide, std::abs((l - r) * 0.5f));
                }

                cachedPeaksL.push_back(pL);
                cachedPeaksR.push_back(pR);
                cachedPeaksMid.push_back(pMid);
                cachedPeaksSide.push_back(pSide);
            }
            else {
                auto rangeL = fileBuffer.findMinMax(0, startSample, numSamplesInPoint);
                pL = std::max(std::abs(rangeL.getStart()), std::abs(rangeL.getEnd()));
                cachedPeaksL.push_back(pL);
            }
        }
    }
};

class Track {
public:
    Track(int id, juce::String n, TrackType t) : trackId(id), name(n), type(t) {
        color = juce::Colour(juce::Random::getSystemRandom().nextFloat(), 0.6f, 0.8f, 1.0f);
    }

    int getId() const { return trackId; }
    juce::String getName() const { return name; }
    void setName(juce::String n) { name = n; }
    TrackType getType() const { return type; }
    juce::Colour getColor() const { return color; }

    float getVolume() const { return volume; }
    void setVolume(float v) { volume = v; }
    float getBalance() const { return balance; }
    void setBalance(float b) { balance = b; }

    WaveformViewMode getWaveformViewMode() const { return waveformViewMode; }
    void setWaveformViewMode(WaveformViewMode mode) { waveformViewMode = mode; }

    juce::StringArray getPluginNames() const { return pluginNames; }
    void addPluginName(juce::String n) { pluginNames.add(n); }

    std::vector<Note> notes;
    juce::OwnedArray<VSTHost> plugins;
    juce::OwnedArray<AudioClipData> audioClips;

    int folderDepthChange = 0;
    int folderDepth = 0;
    bool isCollapsed = false;
    bool isShowingInChildren = true;

    juce::AudioBuffer<float> audioBuffer;
    float currentPeakLevelL = 0.0f;
    float currentPeakLevelR = 0.0f;

private:
    int trackId;
    juce::String name;
    TrackType type;
    juce::Colour color;
    float volume = 0.8f;
    float balance = 0.0f;
    juce::StringArray pluginNames;

    WaveformViewMode waveformViewMode = WaveformViewMode::Combined;
};