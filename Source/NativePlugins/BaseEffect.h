#pragma once
#include <JuceHeader.h>
#include "../Modules/Routing/Data/SidechainData.h"

class TrackContainer; // Forward declaration

enum class PluginRouting { Stereo, Mid, Side };

class BaseEffect {
public:
    virtual ~BaseEffect();

    virtual bool isLoaded() const = 0;
    virtual juce::String getLoadedPluginName() const = 0;

    virtual bool isBypassed() const = 0;
    virtual void setBypassed(bool shouldBypass) = 0;

    virtual void showWindow(TrackContainer* container = nullptr) = 0;

    virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) = 0;
    virtual void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) = 0;
    virtual void reset() = 0; 
    
    virtual void setNonRealtime(bool isNonRealtime); 
    virtual void updatePlayHead(bool isPlaying, int64_t samplePos);
    virtual void setParameterModulation(int index, float delta) {}
    virtual void applyModulations() {}

    virtual void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, const juce::AudioBuffer<float>* sidechainBuffer);
    
    SidechainSettings sidechain;
    virtual bool supportsSidechain() const;
    
    std::function<void()> onSidechainChanged;

    virtual void getStateInformation(juce::MemoryBlock& destData);
    virtual void setStateInformation(const void* data, int sizeInBytes);

    virtual int getLatencySamples() const = 0;
    virtual int getLastKnownLatency() const;
    virtual void setLastKnownLatency(int l);

    virtual PluginRouting getRouting() const;
    virtual void setRouting(PluginRouting r);

    virtual bool isNative() const;
    virtual juce::Component* getCustomEditor();

    bool getIsInstrument() const;
    void setIsInstrument(bool isInst);

protected:
    PluginRouting routing = PluginRouting::Stereo;
    int lastKnownLatency = 0;
    bool isInstrument = false;
};