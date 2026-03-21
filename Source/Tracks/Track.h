#pragma once
#include <JuceHeader.h>
#include "../GlobalData.h" 
#include "../PluginHost/VSTHost.h" 

enum class TrackType { MIDI, Audio };

// Estructura para almacenar el audio cargado en RAM
struct AudioClipData {
    juce::String name;
    float startX = 0.0f;
    float width = 0.0f;
    juce::AudioBuffer<float> fileBuffer;
    double sourceSampleRate = 44100.0;
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

    juce::StringArray getPluginNames() const { return pluginNames; }
    void addPluginName(juce::String n) { pluginNames.add(n); }

    std::vector<Note> notes;
    juce::OwnedArray<VSTHost> plugins;

    // Almacén de clips de audio reales
    juce::OwnedArray<AudioClipData> audioClips;

    // --- REAPER FOLDER MECHANICS ---
    int folderDepthChange = 0;
    int folderDepth = 0;
    bool isCollapsed = false;
    bool isShowingInChildren = true;

    juce::AudioBuffer<float> audioBuffer;

    // --- NIVELES PARA EL MIXER ---
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
};