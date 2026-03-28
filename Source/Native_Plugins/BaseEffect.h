#pragma once
#include <JuceHeader.h>

enum class PluginRouting { Stereo, Mid, Side };

class BaseEffect {
public:
    virtual ~BaseEffect() = default;

    virtual bool isLoaded() const = 0;
    virtual juce::String getLoadedPluginName() const = 0;

    virtual bool isBypassed() const = 0;
    virtual void setBypassed(bool shouldBypass) = 0;

    virtual void showWindow() = 0;

    virtual void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) = 0;
    virtual void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) = 0;

    // --- NUEVO: ANTENA DE RELOJ PARA QUE EL VST3 DIBUJE EL FFT CORRECTAMENTE ---
    virtual void updatePlayHead(bool isPlaying, int64_t samplePos) {}

    virtual int getLatencySamples() const = 0;
    virtual int getLastKnownLatency() const { return lastKnownLatency; }
    virtual void setLastKnownLatency(int l) { lastKnownLatency = l; }

    virtual PluginRouting getRouting() const { return routing; }
    virtual void setRouting(PluginRouting r) { routing = r; }

    virtual bool isNative() const { return false; }
    virtual juce::Component* getCustomEditor() { return nullptr; }

protected:
    PluginRouting routing = PluginRouting::Stereo;
    int lastKnownLatency = 0;
};