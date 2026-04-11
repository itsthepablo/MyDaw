#include "AudioClipRenderer.h"
#include "AudioWaveformRenderer.h"
#include "AudioSpectrogramRenderer.h"
#include <cmath>
#include <algorithm>

void AudioClipRenderer::drawAudioClip(juce::Graphics& g, 
                                     const AudioClip& clip, 
                                     juce::Rectangle<float> area, 
                                     juce::Colour trackColor,
                                     WaveformViewMode viewMode,
                                     double hZoom,
                                     double clipStartSamples,
                                     double viewStartSamples,
                                     AudioClipLookAndFeel* lf)
{
    AudioClipLookAndFeel defaultLF;
    AudioClipLookAndFeel& activeLF = lf ? *lf : defaultLF;

    if (area.getWidth() <= 0 || area.getHeight() <= 0) return;
    const float width = area.getWidth();

    // --- MODO ESPECTROGRAMA PRO (DELEGADO) ---
    if (viewMode == WaveformViewMode::Spectrogram) {
        AudioSpectrogramRenderer::renderSpectrogramMode(g, clip, area, hZoom, viewStartSamples, clipStartSamples);
        return;
    }

    // --- MODO WAVEFORM (ESTANDAR) ---
    
    // 1. Dibujar Fondo y Outline (Redondeo local para nitidez)
    activeLF.drawAudioClipBackground(g, area.toNearestInt().toFloat(), trackColor, clip.getIsSelected(), clip.getStyle());

    // --- LOGICA DE CABECERA "STICKY" ---
    juce::Colour waveColor = activeLF.getWaveformColor(trackColor, clip.getStyle());

    float visibleX = std::max(0.0f, area.getX());
    float stickyWidth = area.getRight() - visibleX;
    
    juce::Rectangle<float> headerRect(visibleX, area.getY(), std::max(0.0f, stickyWidth), 14.0f);

    // 2. Dibujar Cabecera con nombre
    activeLF.drawAudioClipHeader(g, headerRect.toNearestInt().toFloat(), clip.getName(), trackColor, clip.getStyle(), waveColor);

    // 3. Preparar area de dibujo de forma de onda (AREA EXACTA FLOAT ANTI-JITTER)
    juce::Rectangle<float> waveformArea = area.withTrimmedTop(18.0f);

    if (!clip.isLoaded()) {
        g.setColour(trackColor.brighter(0.2f).withAlpha(0.3f));
        g.setFont(juce::Font(11.0f));
        g.drawText("Cargando...", waveformArea, juce::Justification::centred, true);
        return;
    }
    
    // Blindaje de Seguridad y Calculo Matematico Critico
    if (std::isnan(hZoom) || hZoom <= 1e-8) return;

    float baseW = clip.getOriginalWidth() <= 0 ? clip.getWidth() : clip.getOriginalWidth();
    const double samplesPerUnit = (double)clip.getBuffer().getNumSamples() / (double)baseW;
    const double samplesPerPixel = samplesPerUnit / hZoom;

    if (std::isnan(samplesPerPixel) || samplesPerPixel <= 0.0) return;

    const double pixelsPerSample = 1.0 / samplesPerPixel;
    const double visibleStartS = viewStartSamples - clipStartSamples + (clip.getOffsetX() * samplesPerUnit);
    const double visibleEndS = visibleStartS + (double)width * samplesPerPixel;

    // 4. Delegar renderizado de forma de onda
    AudioWaveformRenderer::renderWaveformMode(g, clip, waveformArea, viewMode, 
                                             visibleStartS, visibleEndS, 
                                             pixelsPerSample, samplesPerPixel, 
                                             waveColor);
}

void AudioClipRenderer::drawWaveformSummary(juce::Graphics& g, const AudioClip& clip, juce::Rectangle<float> area, 
    double hZoom, double clipStartUnits, double viewStartUnits) 
{
    AudioWaveformRenderer::drawWaveformSummary(g, clip, area, hZoom, clipStartUnits, viewStartUnits);
}
