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
    virtual void reset() = 0; // NUEVO: resetear ecos/colas sin reasignar RAM
    virtual void setNonRealtime(bool isNonRealtime) {} // NUEVO: para plugins VST3 que soportan HQ mode en export

    // --- NUEVO: ANTENA DE RELOJ PARA QUE EL VST3 DIBUJE EL FFT CORRECTAMENTE ---
    virtual void updatePlayHead(bool isPlaying, int64_t samplePos) {}

    // --- SIDECHAIN SUPPORT ---
    virtual void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, const juce::AudioBuffer<float>* sidechainBuffer) { 
        processBlock(buffer, midiMessages); 
    }
    
    std::atomic<int> sidechainSourceTrackId { -1 };
    virtual bool supportsSidechain() const { return false; }

    // --- STATE MANAGEMENT ---
    virtual void getStateInformation(juce::MemoryBlock& destData) {}
    virtual void setStateInformation(const void* data, int sizeInBytes) {}

    virtual int getLatencySamples() const = 0;
    virtual int getLastKnownLatency() const { return lastKnownLatency; }
    virtual void setLastKnownLatency(int l) { lastKnownLatency = l; }

    virtual PluginRouting getRouting() const { return routing; }
    virtual void setRouting(PluginRouting r) { routing = r; }

    virtual bool isNative() const { return false; }
    virtual juce::Component* getCustomEditor() { return nullptr; }

    bool getIsInstrument() const { return isInstrument; }
    void setIsInstrument(bool isInst) { isInstrument = isInst; }

protected:
    PluginRouting routing = PluginRouting::Stereo;
    int lastKnownLatency = 0;
    bool isInstrument = false;
};