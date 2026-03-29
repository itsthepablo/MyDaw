#pragma once
#include <JuceHeader.h>
#include "../GlobalData.h" 
#include "../Native_Plugins/BaseEffect.h"
#include "../PluginHost/VSTHost.h" 
#include "../Engine/SimpleLoudness.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <algorithm>
#include <cmath>

enum class TrackType { MIDI, Audio };
enum class WaveformViewMode { Combined, SeparateLR, MidSide, Spectrogram }; // Agregado Espectrograma

struct AudioPeak {
    float maxPos = 0.0f;
    float minNeg = 0.0f;
};

struct AudioClipData {
    juce::String name;
    float startX = 0.0f;
    float width = 0.0f;
    float offsetX = 0.0f;
    float originalWidth = 0.0f;
    bool isMuted = false;
    bool isSelected = false;
    juce::String sourceFilePath; 
    juce::AudioBuffer<float> fileBuffer;
    double sourceSampleRate = 44100.0;

    std::vector<AudioPeak> cachedPeaksL;
    std::vector<AudioPeak> cachedPeaksR;
    std::vector<AudioPeak> cachedPeaksMid;
    std::vector<AudioPeak> cachedPeaksSide;
    
    juce::Image spectrogramImage; // Cache Matrix FFT

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

            AudioPeak pL, pR, pMid, pSide;

            if (isStereo) {
                const float* lData = fileBuffer.getReadPointer(0, startSample);
                const float* rData = fileBuffer.getReadPointer(1, startSample);

                for (int s = 0; s < numSamplesInPoint; ++s) {
                    float l = lData[s];
                    float r = rData[s];
                    pL.maxPos = std::max(pL.maxPos, l); pL.minNeg = std::min(pL.minNeg, l);
                    pR.maxPos = std::max(pR.maxPos, r); pR.minNeg = std::min(pR.minNeg, r);
                    
                    float mid = (l + r) * 0.5f;
                    pMid.maxPos = std::max(pMid.maxPos, mid); pMid.minNeg = std::min(pMid.minNeg, mid);
                    
                    float side = (l - r) * 0.5f;
                    pSide.maxPos = std::max(pSide.maxPos, side); pSide.minNeg = std::min(pSide.minNeg, side);
                }
                cachedPeaksL.push_back(pL);
                cachedPeaksR.push_back(pR);
                cachedPeaksMid.push_back(pMid);
                cachedPeaksSide.push_back(pSide);
            }
            else {
                const float* lData = fileBuffer.getReadPointer(0, startSample);
                for (int s = 0; s < numSamplesInPoint; ++s) {
                    float l = lData[s];
                    pL.maxPos = std::max(pL.maxPos, l); pL.minNeg = std::min(pL.minNeg, l);
                }
                cachedPeaksL.push_back(pL);
            }
        }
        
        // --- FFT SPECTROGRAM GENERATION ---
        auto getHeatColor = [](float magnitude) -> juce::Colour {
            magnitude = juce::jlimit(0.0f, 1.0f, magnitude);
            juce::Colour c = juce::Colour(20, 22, 25);
            if (magnitude < 0.2f) return c.interpolatedWith(juce::Colours::darkred, magnitude * 5.0f);
            if (magnitude < 0.6f) return juce::Colours::darkred.interpolatedWith(juce::Colours::orange, (magnitude - 0.2f) / 0.4f);
            return juce::Colours::orange.interpolatedWith(juce::Colours::yellow, (magnitude - 0.6f) / 0.4f);
        };

        const int fftOrder = 10; // 1024 samples (512 Freq Bins)
        const int fftSize = 1 << fftOrder;
        
        juce::dsp::FFT fft(fftOrder);
        juce::dsp::WindowingFunction<float> window((size_t)fftSize, juce::dsp::WindowingFunction<float>::hann);
        
        int totalFrames = numSamples / fftSize;
        if (totalFrames > 0) {
            int imgHeight = fftSize / 2; 
            spectrogramImage = juce::Image(juce::Image::ARGB, totalFrames, imgHeight, true);
            juce::Image::BitmapData bmpData(spectrogramImage, juce::Image::BitmapData::writeOnly);
            
            std::vector<float> fftData((size_t)(fftSize * 2), 0.0f);
            const float* audioData = fileBuffer.getReadPointer(0); 
            
            for (int frame = 0; frame < totalFrames; ++frame) {
                int startSample = frame * fftSize;
                std::fill(fftData.begin(), fftData.end(), 0.0f);
                for (int i = 0; i < fftSize; ++i) {
                    if (startSample + i < numSamples) fftData[i] = audioData[startSample + i];
                }
                
                window.multiplyWithWindowingTable(fftData.data(), (size_t)fftSize);
                fft.performFrequencyOnlyForwardTransform(fftData.data()); 
                
                for (int bin = 0; bin < imgHeight; ++bin) {
                    float rawMag = fftData[bin];
                    float intensity = 0.0f;
                    if (rawMag > 0.0001f) {
                       intensity = juce::jmin(1.0f, (float)std::log10(1.0f + rawMag * 50.0f) / 3.0f);
                    }
                    juce::Colour heat = getHeatColor(intensity);
                    bmpData.setPixelColour(frame, imgHeight - 1 - bin, heat);
                }
            }
        }
    }
};

struct MidiClipData {
    juce::String name;
    float startX = 0.0f;
    float width = 320.0f;
    float offsetX = 0.0f;
    bool isMuted = false;
    bool isSelected = false;
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

    float getPreGain() const { return preGain; }
    void setPreGain(float v) { preGain = v; }

    float getPostGain() const { return postGain; }
    void setPostGain(float v) { postGain = v; }

    bool isPhaseInverted = false;
    bool isMonoActive = false;

    std::vector<Note> notes;

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

    juce::AudioBuffer<float> audioBuffer;
    juce::AudioBuffer<float> instrumentMixBuffer;
    juce::AudioBuffer<float> tempSynthBuffer;
    juce::AudioBuffer<float> midSideBuffer;
    
    // --- PDC (COMPENSACIÓN DE LATENCIA) ---
    juce::AudioBuffer<float> pdcBuffer;
    int pdcWritePtr = 0;
    int currentLatency = 0;
    int requiredDelay = 0;

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