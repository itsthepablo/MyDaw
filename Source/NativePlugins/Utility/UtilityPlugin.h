#pragma once
#include <JuceHeader.h>
#include "../BaseEffect.h"
#include <atomic>

// Forward declaration
#include "UtilityEditor.h"

// ==============================================================================
// MOTOR DSP DEL PLUGIN NATIVO (True Stereo Panner Lineal)
// ==============================================================================
class UtilityPlugin : public BaseEffect {
public:
    UtilityPlugin();
    ~UtilityPlugin() override = default;

    bool isLoaded() const override { return true; }
    juce::String getLoadedPluginName() const override { return "Utility"; }
    bool isBypassed() const override { return bypassed; }
    void setBypassed(bool b) override { bypassed = b; }

    void showWindow(TrackContainer* container = nullptr) override {
        // Los plugins nativos no usan ventana flotante por ahora
    }

    void prepareToPlay(double, int) override {
        // Inicialización de DSP interno si fuera necesario
    }

    void reset() override {
        // No hay colas de memoria que resetear
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override;

    // --- EL UTILITY ES MATEMÁTICA PURA, NO TIENE LATENCIA ---
    int getLatencySamples() const override { return 0; }

    bool isNative() const override { return true; }
    juce::Component* getCustomEditor() override;

private:
    bool bypassed = false;
    std::atomic<float> gain;
    std::atomic<float> pan;
    std::unique_ptr<UtilityEditor> editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UtilityPlugin)
};