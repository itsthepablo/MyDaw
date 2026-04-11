#pragma once
#include <JuceHeader.h>
#include "../AudioClip.h"
#include "../AudioClipStyles.h"
#include "../LookAndFeel/AudioClipLF.h"

/**
 * AudioWaveformRenderer: Gestiona el renderizado vectorial de alta fidelidad.
 * Protege la logica matematica de picos, LODs y dibujo sin jitter.
 */
class AudioWaveformRenderer {
public:
    static void renderWaveformMode(juce::Graphics& g, 
                                 const AudioClip& clip, 
                                 juce::Rectangle<float> area,
                                 WaveformViewMode mode,
                                 double visibleStartS,
                                 double visibleEndS,
                                 double pixelsPerSample,
                                 double samplesPerPixel,
                                 juce::Colour waveColor);

    static void drawWaveformSummary(juce::Graphics& g, 
                                   const AudioClip& clip, 
                                   juce::Rectangle<float> area, 
                                   double hZoom, 
                                   double clipStartUnits, 
                                   double viewStartUnits);

private:
    static double calculateLODLevel(double spp);
    
    static AudioPeak getBlendedPeak(const AudioClip& clip, 
                                   double s, 
                                   int level, 
                                   WaveformViewMode mode, 
                                   int chanIdx);

    static void renderSeamlessLayer(juce::Graphics& g, 
                                   const AudioClip& clip, 
                                   juce::Rectangle<float> area,
                                   double startSample, 
                                   double endSample, 
                                   double pixelsPerSample, 
                                   double spp, 
                                   WaveformViewMode mode, 
                                   int chanIdx, 
                                   juce::Colour color);
};
