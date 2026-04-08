#include "MixerChannelState.h"

MixerChannelState::MixerChannelState(int id, juce::AudioProcessorValueTreeState& vts)
    : trackID(id)
{
    volParam = vts.getRawParameterValue(MixerParameterID::getVolumeID(id));
    panParam = vts.getRawParameterValue(MixerParameterID::getPanID(id));
    muteParam = vts.getRawParameterValue(MixerParameterID::getMuteID(id));
    soloParam = vts.getRawParameterValue(MixerParameterID::getSoloID(id));
}

void MixerChannelState::addParametersToLayout(juce::AudioProcessorValueTreeState::ParameterLayout& layout, int id)
{
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        MixerParameterID::getVolumeID(id), "Vol " + juce::String(id),
        juce::NormalisableRange<float>(0.0f, 1.25f, 0.001f, 0.5f), 1.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        MixerParameterID::getPanID(id), "Pan " + juce::String(id), -1.0f, 1.0f, 0.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        MixerParameterID::getMuteID(id), "Mute " + juce::String(id), false));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        MixerParameterID::getSoloID(id), "Solo " + juce::String(id), false));
}