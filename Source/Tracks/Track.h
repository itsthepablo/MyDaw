#pragma once
#include <JuceHeader.h>
#include "../GlobalData.h" 
#include "../Native_Plugins/BaseEffect.h"
#include "../PluginHost/VSTHost.h" 
#include "../Engine/SimpleLoudness.h"
#include <vector>
#include <algorithm>
#include <cmath>

enum class TrackType { MIDI, Audio };
enum class WaveformViewMode { Combined, SeparateLR, MidSide };

struct AudioClipData {
    juce::String name;
    float startX = 0.0f;
    float width = 0.0f;
    juce::String sourceFilePath; // <-- Agregado para arreglar el error de ProjectManager
    juce::AudioBuffer<float> fileBuffer;
    double sourceSampleRate = 44100.0;

    std::vector<float> cachedPeaksL;
    std::vector<float> cachedPeaksR;
    std::vector<float> cachedPeaksMid;
    std::vector<float> cachedPeaksSide;

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

struct MidiClipData {
    juce::String name;
    float startX = 0.0f;
    float width = 320.0f;
    std::vector<Note> notes;
    juce::Colour color;

    AutoLane autoVol;
    AutoLane autoPan;
    AutoLane autoPitch;
    AutoLane autoFilter;
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

    // --- GAIN STATION STATE ---
    float getPreGain() const { return preGain; }
    void setPreGain(float v) { preGain = v; }

    float getPostGain() const { return postGain; }
    void setPostGain(float v) { postGain = v; }

    bool isPhaseInverted = false;
    bool isMonoActive = false;

    std::vector<Note> notes;

    // --- CAMBIO CLAVE: Ahora acepta BaseEffect para alojar Nativos y VST3 ---
    juce::OwnedArray<BaseEffect> plugins;

    juce::OwnedArray<AudioClipData> audioClips;
    juce::OwnedArray<MidiClipData> midiClips;

    int folderDepthChange = 0;
    int folderDepth = 0;
    bool isCollapsed = false;
    bool isShowingInChildren = true;
    bool isFolderStart = false;
    bool isFolderEnd = false;

    bool isMuted = false;
    bool isSoloed = false;
    bool isSelected = false;
    bool isInlineEditingActive = false;

    // --- PRE-ASIGNACIÓN DE BÚFERES (TIEMPO REAL COMERCIAL) ---
    juce::AudioBuffer<float> audioBuffer;
    juce::AudioBuffer<float> instrumentMixBuffer;
    juce::AudioBuffer<float> tempSynthBuffer;
    juce::AudioBuffer<float> midSideBuffer;

    float currentPeakLevelL = 0.0f;
    float currentPeakLevelR = 0.0f;

    SimpleLoudness preLoudness;
    SimpleLoudness postLoudness;
    bool isAnalyzersPrepared = false;

private:
    int trackId;
    juce::String name;
    TrackType type;
    juce::Colour color;
    float volume = 0.8f;
    float balance = 0.0f;

    float preGain = 1.0f;
    float postGain = 1.0f;

    juce::StringArray pluginNames;

    WaveformViewMode waveformViewMode = WaveformViewMode::Combined;
};