#pragma once
#include <JuceHeader.h>
#include "GlobalData.h" 
#include "SmartColorUtils.h"
#include "../NativePlugins/BaseEffect.h"
#include "../Clips/Audio/AudioClip.h"
#include "../Clips/Midi/MidiPattern.h"
#include "../Tracks/Data/TrackData.h"
#include "../Tracks/Data/TrackSnapshots.h"
#include "../Tracks/DSP/TrackDSP.h"

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

/**
 * @class Track
 * El "God Object" de cada pista. Coordina los datos (Data), el procesado (DSP) 
 * y actúa como modelo para la interfaz (UI).
 */
class Track : public juce::ChangeBroadcaster {
public:
    // --- MÓDULOS DE DATOS ---
    GainStationData   gainStationData;
    MixerChannelData  mixerData;
    LoudnessTrackData loudnessTrackData;
    BalanceTrackData  balanceTrackData;
    MidSideTrackData  midSideTrackData;
    TrackRoutingData  routingData;

    // --- MÓDULO DSP ---
    TrackDSP dsp;

    friend class MixerParameterBridge;
    friend class MixerDSP;

    Track(int id, juce::String n, TrackType t);
    ~Track();

    // --- GETTERS / SETTERS BÁSICOS ---
    int getId() const { return trackId; }
    juce::String getName() const { return name; }
    void setName(juce::String n);
    TrackType getType() const { return type; }
    juce::Colour getColor() const { return color; }
    void setColor(juce::Colour c, bool isManualAssignment = false);

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

    // --- LISTAS DE CONTENIDO ---
    std::vector<Note> notes;
    juce::OwnedArray<BaseEffect> plugins;
    juce::OwnedArray<AudioClip>& getAudioClips() { return audioClips; }
    const juce::OwnedArray<AudioClip>& getAudioClips() const { return audioClips; }
    juce::OwnedArray<MidiPattern>& getMidiClips() { return midiClips; }
    const juce::OwnedArray<MidiPattern>& getMidiClips() const { return midiClips; }

    std::vector<AutomationClipData*> automationClips;

    // --- ESTADO DE JERARQUÍA ---
    int parentId = -1;
    int folderDepth = 0;
    bool isCollapsed = false;
    bool isShowingInChildren = true;

    // --- ESTADO DE UI ---
    bool isSelected = false;
    bool isInlineEditingActive = false;

    // --- BUFFERS DE TRABAJO ---
    juce::AudioBuffer<float> audioBuffer;
    juce::AudioBuffer<float> instrumentMixBuffer;
    juce::AudioBuffer<float> tempSynthBuffer;
    juce::AudioBuffer<float> midSideBuffer;
    juce::SpinLock bufferLock;

    // --- MÉTODOS DE OPERACIÓN ---
    AudioClip* loadAndAddAudioClip(const juce::File& file, float startX);
    void migrateMidiToRelative();
    void commitSnapshot();

    std::atomic<TrackSnapshot*> snapshot { nullptr };
    std::atomic<bool> isLoudnessRecording { true };

private:
    int trackId;
    juce::String name;
    TrackType type;
    juce::Colour color;
    bool isColorManual = false;

    juce::StringArray pluginNames;

    juce::AudioFormatManager thumbnailFormatManager;
    juce::AudioThumbnailCache thumbnailCache { 10 };

    WaveformViewMode waveformViewMode = WaveformViewMode::SeparateLR;
    juce::OwnedArray<AudioClip> audioClips;
    juce::OwnedArray<MidiPattern> midiClips;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Track)
};
