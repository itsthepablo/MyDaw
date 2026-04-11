#pragma once
#include <JuceHeader.h>
#include "../AudioClip.h"
#include "../LookAndFeel/AudioClipLF.h"
#include "../../../Data/GlobalData.h"

/**
 * AudioClipRenderer: Se encarga de dibujar el AudioClip en la pantalla.
 * Utiliza el AudioClipLookAndFeel para la estética.
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

    static void drawSpectrogram(juce::Graphics& g, 
                               const AudioClip& clip, 
                               juce::Rectangle<float> area, 
                               double hZoom, 
                               double pps, 
                               double startSample, 
                               float baseW);
};
