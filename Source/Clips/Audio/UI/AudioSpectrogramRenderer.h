#pragma once
#include <JuceHeader.h>
#include "../AudioClip.h"

/**
 * AudioSpectrogramRenderer: Gestiona el renderizado del espectrograma Pro.
 * Implementa el modo independiente (Black Box) y el mapeo de alta resolucion.
 */
class AudioSpectrogramRenderer {
public:
    static bool renderSpectrogramMode(juce::Graphics& g, 
                                    const AudioClip& clip, 
                                    juce::Rectangle<float> area,
                                    double hZoom,
                                    double viewStartSamples,
                                    double clipStartSamples);

private:
    static void drawSpectrogram(juce::Graphics& g, 
                               const AudioClip& clip, 
                               juce::Rectangle<float> area, 
                               double hZoom, 
                               double pps, 
                               double startSample, 
                               float baseW);
};
