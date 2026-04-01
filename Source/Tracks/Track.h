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
#include <atomic>

enum class TrackType { MIDI, Audio, Folder };
enum class WaveformViewMode { Combined, SeparateLR, MidSide, Spectrogram }; 
enum class SendRouting { Stereo, Mid, Side }; 

struct AudioPeak {
    float maxPos = 0.0f;
    float minNeg = 0.0f;
};

// ============================================================
// ESTRUCTURA DE ENVÍO (SEND) — Ruteo entre pistas
// ============================================================
struct TrackSend {
    int targetTrackId = -1;
    float gain = 1.0f; // Multiplicador de volumen
    bool isPreFader = false;
    bool isMuted = false;
    SendRouting routing = SendRouting::Stereo; 
};

struct AudioClipData {
    juce::String name;
    float startX = 0.0f;
    float width = 0.0f;
    float offsetX = 0.0f;
    float originalWidth = 0.0f;
    bool isMuted = false;
    bool isSelected = false;
    std::atomic<bool> isLoaded { true }; 
    juce::String sourceFilePath; 
    juce::AudioBuffer<float> fileBuffer;
    double sourceSampleRate = 44100.0;

    std::vector<AudioPeak> cachedPeaksL;
    std::vector<AudioPeak> cachedPeaksR;
    std::vector<AudioPeak> cachedPeaksMid;
    std::vector<AudioPeak> cachedPeaksSide;
    
    juce::Image spectrogramImage; 

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
        
        auto getHeatColor = [](float magnitude) -> juce::Colour {
            magnitude = juce::jlimit(0.0f, 1.0f, magnitude);
            juce::Colour c = juce::Colour(20, 22, 25);
            if (magnitude < 0.2f) return c.interpolatedWith(juce::Colours::darkred, magnitude * 5.0f);
            if (magnitude < 0.6f) return juce::Colours::darkred.interpolatedWith(juce::Colours::orange, (magnitude - 0.2f) / 0.4f);
            return juce::Colours::orange.interpolatedWith(juce::Colours::yellow, (magnitude - 0.6f) / 0.4f);
        };

        const int fftOrder = 10;
        const int fftSize = 1 << fftOrder;
        const int hopSize = 256; 
        
        juce::dsp::FFT fft(fftOrder);
        juce::dsp::WindowingFunction<float> window((size_t)fftSize, juce::dsp::WindowingFunction<float>::hann);
        
        int totalFrames = numSamples / hopSize;
        if (totalFrames > 0) {
            int imgHeight = fftSize / 2; 
            spectrogramImage = juce::Image(juce::Image::ARGB, totalFrames, imgHeight, true);
            juce::Image::BitmapData bmpData(spectrogramImage, juce::Image::BitmapData::writeOnly);
            
            std::vector<float> fftData((size_t)(fftSize * 2), 0.0f);
            const float* audioData = fileBuffer.getReadPointer(0); 
            
            for (int frame = 0; frame < totalFrames; ++frame) {
                int startSample = frame * hopSize;
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
                    bmpData.setPixelColour(frame, imgHeight - bin - 1, heat);
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

// ============================================================
// DOUBLE BUFFERING (SNAPSHOTS) — Thread-Safe Audio Access
// ============================================================
struct AudioClipSnapshot {
    float startX     = 0.0f;
    float width      = 0.0f;
    float offsetX    = 0.0f;
    bool  isMuted    = false;
    bool  isLoaded   = true;
    const juce::AudioBuffer<float>* fileBufferPtr = nullptr;
    int   numChannels = 0;
    int   numSamples  = 0;
};

struct MidiNoteSnapshot {
    int   pitch = 0;
    int   x     = 0;
    int   width = 0;
};

struct MidiClipSnapshot {
    float startX  = 0.0f;
    float width   = 320.0f;
    float offsetX = 0.0f;
    bool  isMuted = false;
    std::vector<MidiNoteSnapshot> notes;
};

struct TrackSnapshot {
    std::vector<AudioClipSnapshot> audioClips;
    std::vector<MidiClipSnapshot>  midiClips;
    std::vector<MidiNoteSnapshot>  notes; 
    std::vector<AutomationClipSnapshot> automations;
};

// ============================================================

class Track {
public:
    Track(int id, juce::String n, TrackType t) : trackId(id), name(n), type(t) {
        color = juce::Colour(juce::Random::getSystemRandom().nextFloat(), 0.6f, 0.8f, 1.0f);
        if (t == TrackType::Folder) color = juce::Colour(60, 65, 75); // Color por defecto de carpeta
    }

    ~Track() {
        auto* old = snapshot.exchange(nullptr, std::memory_order_acq_rel);
        delete old;
    }

    int getId() const { return trackId; }
    juce::String getName() const { return name; }
    void setName(juce::String n) { name = n; }
    TrackType getType() const { return type; }
    juce::Colour getColor() const { return color; }
    void setColor(juce::Colour c) { color = c; }

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

    std::atomic<bool> panningModeDual { false };
    std::atomic<float> panL { -1.0f };
    std::atomic<float> panR { 1.0f };

    std::vector<Note> notes;
    juce::OwnedArray<BaseEffect> plugins;

    // --- SENDS (ENVÍOS) ---
    std::vector<TrackSend> sends;
    
    // --- SIDECHAIN DEPENDENCIES ---
    std::vector<int> getSidechainDependencies() const {
        std::vector<int> deps;
        for (auto* p : plugins) {
            if (p != nullptr) {
                int sourceId = p->sidechainSourceTrackId.load(std::memory_order_relaxed);
                if (sourceId != -1) deps.push_back(sourceId);
            }
        }
        return deps;
    }

    juce::OwnedArray<AudioClipData> audioClips;
    juce::OwnedArray<MidiClipData> midiClips;
    std::vector<AutomationClipData*> automationClips; // Referencias a la pool global

    int parentId = -1;
    int folderDepth = 0;
    bool isCollapsed = false;
    bool isShowingInChildren = true;

    bool isMuted = false;
    bool isSoloed = false;
    bool isSelected = false;
    bool isInlineEditingActive = false;

    juce::AudioBuffer<float> audioBuffer;
    juce::AudioBuffer<float> instrumentMixBuffer;
    juce::AudioBuffer<float> tempSynthBuffer;
    juce::AudioBuffer<float> midSideBuffer;
    
    // Proteccion para suma concurrente (Sends + Folders)
    juce::SpinLock bufferLock;

    // --- PDC (COMPENSACIÓN DE LATENCIA) ---
    juce::AudioBuffer<float> pdcBuffer; 
    int pdcWritePtr = 0;
    int currentLatency = 0;
    int requiredDelay = 0;

    void allocatePdcBuffer()
    {
        if (pdcBuffer.getNumSamples() >= 524288) return; 
        pdcBuffer.setSize(2, 524288, false, true, false);
        pdcBuffer.clear();
        pdcWritePtr = 0;
    }

    float currentPeakLevelL = 0.0f;
    float currentPeakLevelR = 0.0f;

    SimpleLoudness preLoudness;
    SimpleLoudness postLoudness;
    bool isAnalyzersPrepared = false;

    AudioClipData* loadAndAddAudioClip(const juce::File& file, float startX)
    {
        juce::AudioFormatManager manager;
        manager.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(file));

        if (reader != nullptr)
        {
            auto* clip = new AudioClipData();
            clip->name = file.getFileNameWithoutExtension();
            clip->sourceFilePath = file.getFullPathName();
            clip->startX = startX;
            
            double duration = (double)reader->lengthInSamples / reader->sampleRate;
            clip->width = (float)(duration * 160.0); 
            clip->originalWidth = clip->width;
            clip->sourceSampleRate = reader->sampleRate;

            clip->fileBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
            reader->read(&clip->fileBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

            clip->generateCache();
            audioClips.add(clip);
            commitSnapshot();
            return clip;
        }
        return nullptr;
    }

    std::atomic<TrackSnapshot*> snapshot { nullptr };

    void commitSnapshot()
    {
        auto* snap = new TrackSnapshot();

        snap->audioClips.reserve((size_t)audioClips.size());
        for (auto* ac : audioClips) {
            if (!ac) continue;
            AudioClipSnapshot acs;
            acs.startX        = ac->startX;
            acs.width         = ac->width;
            acs.offsetX       = ac->offsetX;
            acs.isMuted       = ac->isMuted;
            acs.isLoaded      = ac->isLoaded.load(std::memory_order_relaxed);
            acs.fileBufferPtr = &ac->fileBuffer;   
            acs.numChannels   = ac->fileBuffer.getNumChannels();
            acs.numSamples    = ac->fileBuffer.getNumSamples();
            snap->audioClips.push_back(acs);
        }

        snap->midiClips.reserve((size_t)midiClips.size());
        for (auto* mc : midiClips) {
            if (!mc) continue;
            MidiClipSnapshot mcs;
            mcs.startX  = mc->startX;
            mcs.width   = mc->width;
            mcs.offsetX = mc->offsetX;
            mcs.isMuted = mc->isMuted;
            mcs.notes.reserve(mc->notes.size());
            for (const auto& n : mc->notes) {
                mcs.notes.push_back({ n.pitch, n.x, n.width });
            }
            snap->midiClips.push_back(std::move(mcs));
        }

        snap->notes.reserve(notes.size());
        for (const auto& n : notes) {
            snap->notes.push_back({ n.pitch, n.x, n.width });
        }

        snap->automations.reserve(automationClips.size());
        for (auto* a : automationClips) {
            if (!a) continue;
            AutomationClipSnapshot as;
            as.parameterId = a->parameterId;
            as.lane = a->lane;      // Copia por valor segura en real-time
            snap->automations.push_back(std::move(as));
        }

        auto* old = snapshot.exchange(snap, std::memory_order_acq_rel);
        if (old) {
            juce::MessageManager::callAsync([old]() { delete old; });
        }
    }

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