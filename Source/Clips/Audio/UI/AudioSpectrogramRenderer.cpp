#include "AudioSpectrogramRenderer.h"
#include <cmath>
#include <algorithm>

bool AudioSpectrogramRenderer::renderSpectrogramMode(juce::Graphics& g, 
                                                    const AudioClip& clip, 
                                                    juce::Rectangle<float> area,
                                                    double hZoom,
                                                    double viewStartSamples,
                                                    double clipStartSamples)
{
    // --- MODO ESPECTROGRAMA PRO (BYPASS DE ESTILOS Y COLORES) ---
    // 1. Fondo Negro Profesional
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(area.toNearestInt().toFloat(), 5.0f);

    // 2. Cabezal Negro (Sticky)
    float vX = std::max(0.0f, area.getX());
    float sW = area.getRight() - vX;
    juce::Rectangle<float> hRect(vX, area.getY(), std::max(0.0f, sW), 14.0f);
    
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(hRect.toNearestInt().toFloat(), 5.0f);

    g.setColour(juce::Colours::lightgrey);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(" " + clip.getName(), hRect.reduced(3.0f, 0.0f).toNearestInt(), juce::Justification::centredLeft, true);

    // 3. Contenido (Espectrograma)
    juce::Rectangle<float> specArea = area.withTrimmedTop(18.0f);
    
    // Disparar generación en segundo plano si no existe
    ((AudioClip&)clip).ensureSpectrogramIsGenerated();

    if (clip.isLoaded() && clip.isSpectrogramValid() && !std::isnan(hZoom) && hZoom > 1e-8) {
        float bW = clip.getOriginalWidth() <= 0 ? clip.getWidth() : clip.getOriginalWidth();
        const double sPU = (double)clip.getBuffer().getNumSamples() / (double)bW;
        const double sPP = sPU / hZoom;
        if (sPP > 0.0) {
            const double pPS = 1.0 / sPP;
            const double vS0 = viewStartSamples - clipStartSamples + (clip.getOffsetX() * sPU);
            drawSpectrogram(g, clip, specArea, hZoom, pPS, vS0, bW);
        }
    } else {
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.setFont(juce::Font(11.0f));
        juce::String status = clip.isSpectrogramLoading() ? "Analizando Frecuencias Pro..." : "Esperando Generacion...";
        g.drawText(status, specArea, juce::Justification::centred, true);
    }

    // Borde de seleccion
    if (clip.getIsSelected()) {
        g.setColour(juce::Colours::yellow);
        g.drawRoundedRectangle(area.toNearestInt().toFloat(), 5.0f, 1.5f);
    }
    return true;
}

void AudioSpectrogramRenderer::drawSpectrogram(juce::Graphics& g, const AudioClip& clip, juce::Rectangle<float> area, 
    double hZoom, double pps, double startSample, float baseW) 
{
    const auto& img = clip.getBuffer().getNumSamples() > 0 ? ((AudioClip&)clip).getSpectrogram() : juce::Image();
    if (img.isNull()) return;

    // Usar el hopSize adaptativo calculado durante la generacion
    const double hopSize = clip.getSpectrogramHopSize();
    
    double srcX = startSample / hopSize;
    double srcW = (double)area.getWidth() / (pps * hopSize);

    if (srcW > 0)
    {
        juce::Graphics::ScopedSaveState ss(g);
        g.reduceClipRegion(area.toNearestInt());
        g.setImageResamplingQuality(juce::Graphics::mediumResamplingQuality);
        
        float scaleX = area.getWidth() / (float)srcW;
        float scaleY = area.getHeight() / (float)img.getHeight();
        
        auto trans = juce::AffineTransform::translation (-(float)srcX, 0.0f)
                        .scaled (scaleX, scaleY)
                        .translated (area.getX(), area.getY());
                        
        g.drawImageTransformed (img, trans);
    }
}
