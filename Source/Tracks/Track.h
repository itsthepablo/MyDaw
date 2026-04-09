#pragma once
#include <JuceHeader.h>
#include "../GlobalData.h" 
#include "../Native_Plugins/BaseEffect.h"
#include "../PluginHost/VSTHost.h" 
#include "../Engine/SimpleLoudness.h"
#include "../Engine/SimpleBalance.h"
#include "../Engine/SimpleMidSide.h"
#include "AudioClipData.h"
#include "../UI/MidiPatternStyles.h"
#include <juce_dsp/juce_dsp.h>
#include "../Native_Plugins/ChannelEQ/ChannelEQ_DSP.h"
#include "../Mixer/Data/MixerChannelData.h"
#include "../Modules/GainStation/Data/GainStationData.h"
#include "../Modules/GainStation/Data/GainStationData.h"
#include "../Modules/LoudnessTrack/Data/LoudnessTrackData.h"
#include "../Modules/BalanceTrack/Data/BalanceTrackData.h"
#include "../Modules/MidSideTrack/Data/MidSideTrackData.h"
#include <map>
#include <vector>
#include <algorithm>
#include <cmath>
#include <atomic>

enum class TrackType { MIDI, Audio, Folder, Loudness, Balance, MidSide };
enum class WaveformViewMode { Combined, SeparateLR, MidSide, Spectrogram }; 
enum class SendRouting { Stereo, Mid, Side }; 

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

struct MidiClipData {
    juce::String name;
    float startX = 0.0f;
    float width = 320.0f;
    float offsetX = 0.0f;
    bool isMuted = false;
    bool isSelected = false;
    std::vector<Note> notes;
    juce::Colour color;
    MidiStyleType style = MidiStyleType::Classic;

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
    int    pitch     = 0;
    int    x         = 0;
    int    width     = 0;
    double frequency = 0.0;
};

struct MidiClipSnapshot {
    float startX  = 0.0f;
    float width   = 320.0f;
    float offsetX = 0.0f;
    bool  isMuted = false;
    MidiStyleType style = MidiStyleType::Classic;
    std::vector<MidiNoteSnapshot> notes;
};

// ============================================================
// LOUDNESS HISTORY — Almacenamiento de la curva para la pista
// ============================================================
// Estructuras de historial movidas a LoudnessTrackData.h

// Histories movidas a LoudnessTrackData.h

// ============================================================
struct TrackSnapshot {
    std::vector<AudioClipSnapshot> audioClips;
    std::vector<MidiClipSnapshot>  midiClips;
    std::vector<MidiNoteSnapshot>  notes; 
    std::vector<AutomationClipSnapshot> automations;
};

// ============================================================

class Track {
public:
    GainStationData gainStationData;
    MixerChannelData mixerData;
    LoudnessTrackData loudnessTrackData;
    BalanceTrackData  balanceTrackData;
    MidSideTrackData  midSideTrackData;

    friend class MixerParameterBridge;
    friend class MixerDSP;
    Track(int id, juce::String n, TrackType t) : trackId(id), name(n), type(t) {
        color = juce::Colour(juce::Random::getSystemRandom().nextFloat(), 0.6f, 0.8f, 1.0f);
        if (t == TrackType::Folder) color = juce::Colour(60, 65, 75);
        thumbnailFormatManager.registerBasicFormats();
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

    float getVolume() const { return mixerData.volume; }
    void setVolume(float v) { mixerData.setVolume(v); }
    float getBalance() const { return mixerData.balance; }
    void setBalance(float b) { mixerData.setBalance(b); }
    
    float getPreGain() const { return gainStationData.preGain.load(std::memory_order_relaxed); }
    void setPreGain(float v) { gainStationData.preGain.store(v, std::memory_order_relaxed); }
    float getPostGain() const { return gainStationData.postGain.load(std::memory_order_relaxed); }
    void setPostGain(float v) { gainStationData.postGain.store(v, std::memory_order_relaxed); }

    void prepare(double sampleRate, int samplesPerBlock)
    {
        mixerData.prepare(sampleRate, samplesPerBlock);
        inlineEQ.prepare(sampleRate, samplesPerBlock, 2);
    }

    WaveformViewMode getWaveformViewMode() const { return waveformViewMode; }
    void setWaveformViewMode(WaveformViewMode mode) { waveformViewMode = mode; }

    juce::StringArray getPluginNames() const { return pluginNames; }
    void addPluginName(juce::String n) { pluginNames.add(n); }

    bool isPhaseInverted() const { return gainStationData.isPhaseInverted.load(std::memory_order_relaxed); }
    void setPhaseInverted(bool i) { gainStationData.isPhaseInverted.store(i, std::memory_order_relaxed); }
    bool isMonoActive() const { return gainStationData.isMonoActive.load(std::memory_order_relaxed); }
    void setMonoActive(bool m) { gainStationData.isMonoActive.store(m, std::memory_order_relaxed); }


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

    // bool isMuted -> MOVIDO A mixerData
    // bool isSoloed -> MOVIDO A mixerData
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

    // currentPeakLevelL/R -> MOVIDO A mixerData

    SimpleLoudness preLoudness;
    SimpleLoudness postLoudness;
    SimpleBalance  postBalance;
    SimpleMidSide  postMidSide;
    
    ChannelEQ_DSP inlineEQ;

    bool isAnalyzersPrepared = false;

    // --- LOUDNESS/ANALYSIS TRACK SPECIFIC ---
    std::atomic<bool> isLoudnessRecording { true };

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

            // Generar AudioThumbnail para renderizado estable (sin temblor al hacer zoom)
            clip->thumbnail = std::make_unique<juce::AudioThumbnail>(64, thumbnailFormatManager, thumbnailCache);
            clip->thumbnail->reset(clip->fileBuffer.getNumChannels(),
                                   clip->sourceSampleRate,
                                   (juce::int64)clip->fileBuffer.getNumSamples());
            clip->thumbnail->addBlock(0, clip->fileBuffer, 0, clip->fileBuffer.getNumSamples());

            audioClips.add(clip);
            commitSnapshot();
            return clip;
        }
        return nullptr;
    }

    // --- MIGRACIÓN DE COORDENADAS (Legacy support) ---
    void migrateMidiToRelative()
    {
        bool changed = false;
        for (auto* clip : midiClips) {
            if (!clip) continue;
            for (auto& note : clip->notes) {
                // Heurística: si la nota está en el rango absoluto [startX, startX + width], 
                // la convertimos a relativa restando startX.
                if (note.x >= (int)clip->startX && note.x < (int)(clip->startX + clip->width + 1.0f)) {
                    note.x -= (int)clip->startX;
                    changed = true;
                }
            }
        }
        if (changed) commitSnapshot();
    }

    std::atomic<TrackSnapshot*> snapshot { nullptr };

    void commitSnapshot()
    {
        // PDC: alocar buffer bajo demanda — solo si el track tiene contenido.
        // Tracks vacios no consumen los 4MB del ring buffer de retardo.
        if (!audioClips.isEmpty() || !midiClips.isEmpty() || !plugins.isEmpty() || !notes.empty())
            allocatePdcBuffer();

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
            mcs.style   = mc->style;
            mcs.notes.reserve(mc->notes.size());
            for (const auto& n : mc->notes) {
                mcs.notes.push_back({ n.pitch, n.x, n.width, n.frequency });
            }
            snap->midiClips.push_back(std::move(mcs));
        }

        snap->notes.reserve(notes.size());
        for (const auto& n : notes) {
            snap->notes.push_back({ n.pitch, n.x, n.width, n.frequency });
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

    juce::StringArray pluginNames;

    // AudioThumbnail para renderizado estable de waveform
    juce::AudioFormatManager thumbnailFormatManager;
    juce::AudioThumbnailCache thumbnailCache { 10 };

    WaveformViewMode waveformViewMode = WaveformViewMode::Combined;
};