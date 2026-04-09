#pragma once
#include <JuceHeader.h>
#include "../GlobalData.h" 
#include "../Native_Plugins/BaseEffect.h"
#include "../Engine/SimpleLoudness.h"
#include "../Engine/SimpleBalance.h"
#include "../Engine/SimpleMidSide.h"
#include "Data/AudioClipData.h"
#include "../UI/MidiPatternStyles.h"
#include <juce_dsp/juce_dsp.h>
#include "../Native_Plugins/ChannelEQ/ChannelEQ_DSP.h"
#include "../Mixer/Data/MixerChannelData.h"
#include "../Modules/GainStation/Data/GainStationData.h"
#include "../Modules/LoudnessTrack/Data/LoudnessTrackData.h"
#include "../Modules/BalanceTrack/Data/BalanceTrackData.h"
#include "../Modules/MidSideTrack/Data/MidSideTrackData.h"
#include "../Modules/Routing/Data/RoutingData.h"
#include <vector>
#include <atomic>

// Forward declarations to reduce include bloat
class BaseEffect;
struct AutomationClipData;

enum class TrackType { MIDI, Audio, Folder, Loudness, Balance, MidSide };
enum class WaveformViewMode { Combined, SeparateLR, MidSide, Spectrogram }; 

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

struct TrackSnapshot {
    std::vector<AudioClipSnapshot> audioClips;
    std::vector<MidiClipSnapshot>  midiClips;
    std::vector<MidiNoteSnapshot>  notes; 
    std::vector<AutomationClipSnapshot> automations;
};

class Track {
public:
    GainStationData gainStationData;
    MixerChannelData mixerData;
    LoudnessTrackData loudnessTrackData;
    BalanceTrackData  balanceTrackData;
    MidSideTrackData  midSideTrackData;
    TrackRoutingData  routingData;

    friend class MixerParameterBridge;
    friend class MixerDSP;

    Track(int id, juce::String n, TrackType t);
    ~Track();

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

    void prepare(double sampleRate, int samplesPerBlock);

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

    juce::OwnedArray<AudioClipData> audioClips;
    juce::OwnedArray<MidiClipData> midiClips;
    std::vector<AutomationClipData*> automationClips;

    int parentId = -1;
    int folderDepth = 0;
    bool isCollapsed = false;
    bool isShowingInChildren = true;

    bool isSelected = false;
    bool isInlineEditingActive = false;

    juce::AudioBuffer<float> audioBuffer;
    juce::AudioBuffer<float> instrumentMixBuffer;
    juce::AudioBuffer<float> tempSynthBuffer;
    juce::AudioBuffer<float> midSideBuffer;
    
    juce::SpinLock bufferLock;

    juce::AudioBuffer<float> pdcBuffer; 
    int pdcWritePtr = 0;
    int currentLatency = 0;
    int requiredDelay = 0;

    void allocatePdcBuffer();

    SimpleLoudness preLoudness;
    SimpleLoudness postLoudness;
    SimpleBalance  postBalance;
    SimpleMidSide  postMidSide;
    
    ChannelEQ_DSP inlineEQ;

    bool isAnalyzersPrepared = false;

    std::atomic<bool> isLoudnessRecording { true };

    AudioClipData* loadAndAddAudioClip(const juce::File& file, float startX);
    void migrateMidiToRelative();
    void commitSnapshot();

    std::atomic<TrackSnapshot*> snapshot { nullptr };

private:
    int trackId;
    juce::String name;
    TrackType type;
    juce::Colour color;

    juce::StringArray pluginNames;

    juce::AudioFormatManager thumbnailFormatManager;
    juce::AudioThumbnailCache thumbnailCache { 10 };

    WaveformViewMode waveformViewMode = WaveformViewMode::Combined;
};