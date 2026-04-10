#pragma once
#include <JuceHeader.h>

// ==============================================================================
// 1. ESTRUCTURA DE DATOS GLOBAL (Desconectada de las Pistas)
// ==============================================================================
struct RackChannel {
    juce::String name;
    float volume = 0.8f;
    float pan = 0.0f;
    bool isMuted = false;
    bool steps[16] = { false };
    juce::Colour color = juce::Colour(90, 95, 100);
};
