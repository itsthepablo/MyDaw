#pragma once
#include <JuceHeader.h>

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

    // --- MÉTODOS PARA DETECTAR Y DIBUJAR PLUGINS NATIVOS ---
    virtual bool isNative() const { return false; }
    virtual juce::Component* getCustomEditor() { return nullptr; }
};