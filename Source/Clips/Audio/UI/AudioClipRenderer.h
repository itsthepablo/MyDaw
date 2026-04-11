#pragma once
#include <JuceHeader.h>
#include "../AudioClip.h"
#include "../LookAndFeel/AudioClipLF.h"

/**
 * AudioClipRenderer: Orquestador principal de renderizado de clips.
 * Delega el trabajo pesado a renderizadores especializados.
 */
class AudioClipRenderer {
public:
    static void drawAudioClip(juce::Graphics& g, 
                             const AudioClip& clip, 
                             juce::Rectangle<float> area, 
                             juce::Colour trackColor,
                             WaveformViewMode viewMode,
                             double hZoom,
                             double clipStartSamples,
                             double viewStartSamples,
                             AudioClipLookAndFeel* lf = nullptr);

    static void drawWaveformSummary(juce::Graphics& g, 
                                   const AudioClip& clip, 
                                   juce::Rectangle<float> area, 
                                   double hZoom, 
                                   double clipStartUnits, 
                                   double viewStartUnits);
};
