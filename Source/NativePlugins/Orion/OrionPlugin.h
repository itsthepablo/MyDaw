#pragma once
#include <JuceHeader.h>
#include "../BaseEffect.h"
#include "OrionSynth.h"
#include <atomic>
#include <map>

// Forward declarations
#include "OrionEditor.h"

// ==============================================================================
class OrionPlugin : public BaseEffect {
public:
    OrionPlugin();
    ~OrionPlugin() override = default;

    bool isLoaded() const override { return true; }
    juce::String getLoadedPluginName() const override { return "Orion"; }
    bool isBypassed() const override { return bypassed; }
    void setBypassed(bool b) override { bypassed = b; }

    void showWindow(TrackContainer*) override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void reset() override;

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        processBlock(buffer, midiMessages, nullptr);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, const juce::AudioBuffer<float>* sidechainBuffer) override;

    int getLatencySamples() const override { return 0; }
    bool isNative() const override { return true; }
    juce::Component* getCustomEditor() override;

private:
    bool bypassed = false;
    juce::Synthesiser synth;
    juce::MidiKeyboardState keyboardState;
    std::unique_ptr<OrionDebugStats> debugStats; // Usar heap para evitar problemas de forward declaration
    
    std::map<juce::String, std::atomic<float>> paramValues;
    std::map<juce::String, std::atomic<float>*> paramPtrs;

    std::unique_ptr<OrionEditor> editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrionPlugin)
};
