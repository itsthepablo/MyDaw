#pragma once
#include <JuceHeader.h>
#include <atomic>

// ==============================================================================
// MixerParameterID - Identificadores únicos para el sistema de parámetros.
// ==============================================================================
namespace MixerParameterID
{
    inline juce::String getVolumeID(int id) { return "trk_" + juce::String(id) + "_vol"; }
    inline juce::String getPanID(int id) { return "trk_" + juce::String(id) + "_pan"; }
    inline juce::String getMuteID(int id) { return "trk_" + juce::String(id) + "_mute"; }
    inline juce::String getSoloID(int id) { return "trk_" + juce::String(id) + "_solo"; }
}

// ==============================================================================
// MixerChannelState — LA VERDAD ABSOLUTA (El Puente)
// ==============================================================================
class MixerChannelState
{
public:
    MixerChannelState(int id, juce::AudioProcessorValueTreeState& vts);
    ~MixerChannelState() = default;

    float getVolume() const noexcept { return volParam ? volParam->load() : 1.0f; }
    float getPan()    const noexcept { return panParam ? panParam->load() : 0.0f; }
    bool  isMuted()   const noexcept { return muteParam ? muteParam->load() > 0.5f : false; }
    bool  isSoloed()  const noexcept { return soloParam ? soloParam->load() > 0.5f : false; }

    static void addParametersToLayout(juce::AudioProcessorValueTreeState::ParameterLayout& layout, int id);

private:
    int trackID;
    std::atomic<float>* volParam = nullptr;
    std::atomic<float>* panParam = nullptr;
    std::atomic<float>* muteParam = nullptr;
    std::atomic<float>* soloParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelState)
};