#include "AudioClipRenderer.h"

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
    // Carga de LookAndFeel fallback si no se provee uno
    AudioClipLookAndFeel defaultLF;
    AudioClipLookAndFeel& activeLF = lf ? *lf : defaultLF;

    const int width = (int)area.getWidth();
    if (width <= 0 || area.getHeight() <= 0) return;

    // 1. Dibujar Fondo y Outline mediante LookAndFeel (siempre, incluso cargando)
    activeLF.drawAudioClipBackground(g, area, trackColor, clip.getIsSelected());

    // 2. Dibujar Cabecera con nombre (siempre)
    juce::Rectangle<float> headerRect = area.withHeight(14.0f);
    activeLF.drawAudioClipHeader(g, headerRect, clip.getName(), trackColor);

    // 3. Preparar área de dibujo de forma de onda
    juce::Rectangle<float> waveformArea = area.withTrimmedTop(18.0f);

    if (!clip.isLoaded()) {
        // Clip visible con aspecto correcto, sin waveform todavía
        g.setColour(trackColor.brighter(0.2f).withAlpha(0.3f));
        g.setFont(juce::Font(11.0f));
        g.drawText("Cargando...", waveformArea, juce::Justification::centred, true);
        return;
    }
    
    // Blindaje de Seguridad
    if (std::isnan(hZoom) || hZoom <= 1e-8) return;

    float baseW = clip.getOriginalWidth() <= 0 ? clip.getWidth() : clip.getOriginalWidth();
    const double samplesPerUnit = (double)clip.getBuffer().getNumSamples() / (double)baseW;
    const double samplesPerPixel = samplesPerUnit / hZoom;

    if (std::isnan(samplesPerPixel) || samplesPerPixel <= 0.0) return;

    const double pixelsPerSample = 1.0 / samplesPerPixel;
    const double visibleStartS = viewStartSamples - clipStartSamples + (clip.getOffsetX() * samplesPerUnit);
    const double visibleEndS = visibleStartS + (double)width * samplesPerPixel;

    // --- MODO ESPECTROGRAMA ---
    if (viewMode == WaveformViewMode::Spectrogram) {
        drawSpectrogram(g, clip, waveformArea, hZoom, pixelsPerSample, visibleStartS, baseW);
        return;
    }

    // --- RENDERIZADO DE FORMA DE ONDA ---
    juce::Colour waveColor = activeLF.getWaveformColor(trackColor);
    g.setColour(waveColor);
    
    if (viewMode == WaveformViewMode::SeparateLR && clip.getBuffer().getNumChannels() > 1) {
        float h = waveformArea.getHeight() / 2.0f;
        renderSeamlessLayer(g, clip, waveformArea.withHeight(h), visibleStartS, visibleEndS, pixelsPerSample, samplesPerPixel, WaveformViewMode::SeparateLR, 0, waveColor);
        renderSeamlessLayer(g, clip, waveformArea.withY(waveformArea.getY() + h).withHeight(h), visibleStartS, visibleEndS, pixelsPerSample, samplesPerPixel, WaveformViewMode::SeparateLR, 1, waveColor);
    } else {
        renderSeamlessLayer(g, clip, waveformArea, visibleStartS, visibleEndS, pixelsPerSample, samplesPerPixel, viewMode, 0, waveColor);
    }
}

double AudioClipRenderer::calculateLODLevel(double spp) {
    if (spp <= 1.0) return -1.0; 
    if (spp <= 256.0) return (std::log(spp) / std::log(256.0)) - 1.0; 
    return std::log(spp / 256.0) / std::log(4.0);
}

AudioPeak AudioClipRenderer::getBlendedPeak(const AudioClip& clip, double s, int level, WaveformViewMode mode, int chanIdx) {
    if (level < -1) level = -1;
    if (level > 3) level = 3;

    if (level < 0) {
        const auto& buffer = clip.getBuffer();
        int n = buffer.getNumSamples();
        int i0 = (int)std::floor(s), i1 = i0 + 1;
        float frac = (float)(s - i0);
        i0 = std::max(0, std::min(n - 1, i0)); i1 = std::max(0, std::min(n - 1, i1));
        auto* dataL = buffer.getReadPointer(0);
        auto* dataR = (buffer.getNumChannels() > 1) ? buffer.getReadPointer(1) : dataL;
        auto getV = [&](const float* data) { return data[i0] * (1.0f - frac) + data[i1] * frac; };
        if (mode == WaveformViewMode::MidSide) {
            float v = (chanIdx == 0) ? (getV(dataL) + getV(dataR)) * 0.5f : (getV(dataL) - getV(dataR)) * 0.5f;
            return { v, v };
        }
        float val = (chanIdx == 0) ? getV(dataL) : getV(dataR);
        return { val, val };
    }

    bool useMS = (mode == WaveformViewMode::MidSide);
    const std::vector<AudioPeak>* vec = nullptr;
    if (useMS) vec = (chanIdx == 0) ? &clip.getPeaksMid(level) : &clip.getPeaksSide(level);
    else vec = (chanIdx == 0) ? &clip.getPeaksL(level) : &clip.getPeaksR(level);

    if (!vec || vec->empty()) return { 0.0f, 0.0f };
    
    double sppLOD = (level == 0) ? 256.0 : (level == 1) ? 1024.0 : (level == 2) ? 4096.0 : 16384.0;
    double idx = s / sppLOD;
    int i0 = (int)std::floor(idx);
    int i1 = i0 + 1;
    float frac = (float)(idx - i0);
    i0 = std::max(0, std::min((int)vec->size() - 1, i0));
    i1 = std::max(0, std::min((int)vec->size() - 1, i1));
    return { (*vec)[i0].maxPos * (1.0f - frac) + (*vec)[i1].maxPos * frac, (*vec)[i0].minNeg * (1.0f - frac) + (*vec)[i1].minNeg * frac };
}

void AudioClipRenderer::renderSeamlessLayer(juce::Graphics& g, const AudioClip& clip, juce::Rectangle<float> area,
    double startSample, double endSample, double pixelsPerSample, double spp, WaveformViewMode mode, int chanIdx, juce::Colour color)
{
    juce::Graphics::ScopedSaveState ss(g);
    const float midY = area.getCentreY();
    const float halfH = area.getHeight() / 2.0f;
    const bool isStereo = clip.getBuffer().getNumChannels() > 1;

    // Culling
    const float minX = area.getX() - 10.0f;
    const float maxX = area.getRight() + 10.0f;
    auto clampY = [&](float y) { return juce::jlimit(area.getY(), area.getBottom(), y); };

    // --- PARCHE ANTI-LAG (CLIPPING) ---
    const juce::Rectangle<int> clipBounds = g.getClipBounds();
    const float xOff = area.getX();
    double visibleS0 = startSample + (double)(clipBounds.getX() - xOff) / pixelsPerSample - (2.0 / pixelsPerSample);
    double visibleS1 = startSample + (double)(clipBounds.getRight() - xOff) / pixelsPerSample + (2.0 / pixelsPerSample);

    if (spp < 2.0) {
        // Renderizado Micro (Líneas entre muestras)
        const auto& buffer = clip.getBuffer();
        const float* dataL = buffer.getReadPointer(0);
        const float* dataR = (isStereo && (chanIdx == 1 || mode == WaveformViewMode::Combined)) ? buffer.getReadPointer(1) : nullptr;
        const float* activeData = (chanIdx == 1 && dataR) ? dataR : dataL;

        long long s0 = (long long)std::floor(startSample);
        long long s1 = (long long)std::ceil(endSample);
        
        // Aplicar recorte al bucle micro
        s0 = std::max(s0, (long long)std::floor(visibleS0));
        s1 = std::min(s1, (long long)std::ceil(visibleS1));

        float lastX = 0.0f, lastY = 0.0f;
        bool first = true;
        
        for (long long i = s0; i <= s1; ++i) {
            if (i < 0 || i >= buffer.getNumSamples()) continue;
            float val = activeData[i];
            if (isStereo && mode == WaveformViewMode::Combined && dataR) {
                float valR = dataR[i];
                val = (std::abs(val) > std::abs(valR)) ? val : valR;
            }
            float px = (float)((double)(i - startSample) * pixelsPerSample) + area.getX();
            float py = clampY(midY - (val * halfH * 0.95f));
            if (!first && px > minX && lastX < maxX) g.drawLine(lastX, lastY, px, py, 1.5f);
            
            if (pixelsPerSample > 6.0f) {
                float dotSize = std::min(6.0f, (float)(pixelsPerSample * 0.3f));
                if (dotSize > 2.0f && area.contains(px, py)) g.fillEllipse(px - dotSize/2.0f, py - dotSize/2.0f, dotSize, dotSize);
            }
            lastX = px; lastY = py; first = false;
        }
        return;
    }

    // Renderizado Macro (Mip-Maps)
    double lodVal = calculateLODLevel(spp);
    int levelFine = (int)std::floor(lodVal);
    int levelCoarse = levelFine + 1;
    float alpha = (float)(lodVal - levelFine);

    double sppLOD = (levelFine < 0) ? 1.0 : (levelFine == 0) ? 256.0 : (levelFine == 1) ? 1024.0 : (levelFine == 2) ? 4096.0 : 16384.0;
    long long idxStart = (long long)std::floor(startSample / sppLOD) - 1;
    long long idxEnd = (long long)std::ceil(endSample / sppLOD) + 1;

    // Aplicar recorte al bucle macro (picos)
    idxStart = std::max(idxStart, (long long)std::floor(visibleS0 / sppLOD) - 1);
    idxEnd = std::min(idxEnd, (long long)std::ceil(visibleS1 / sppLOD) + 1);

    // --- MODO ESCUDO CONDICIONAL ---
    // Solo activamos la agregación si estamos en el "Valle de la Muerte" (levelFine < 0 y spp >= 2.0)
    // donde hay demasiadas muestras para dibujar una a una pero no hay Mipmaps disponibles.
    if (levelFine < 0 && spp >= 2.0) {
        float lastPx = -1e9f;
        float currentY0 = midY;
        float currentY1 = midY;
        bool hasPending = false;

        for (long long i = idxStart; i < idxEnd; ++i) {
            double s = (double)i * sppLOD;
            AudioPeak pFine = getBlendedPeak(clip, s, levelFine, mode, chanIdx);
            AudioPeak pCoarse = getBlendedPeak(clip, s, levelCoarse, mode, chanIdx);
            float maxV = pFine.maxPos * (1.0f - alpha) + pCoarse.maxPos * alpha;
            float minV = pFine.minNeg * (1.0f - alpha) + pCoarse.minNeg * alpha;

            if (isStereo && mode == WaveformViewMode::Combined) {
                AudioPeak pfR = getBlendedPeak(clip, s, levelFine, mode, 1);
                AudioPeak pcR = getBlendedPeak(clip, s, levelCoarse, mode, 1);
                maxV = std::max(maxV, pfR.maxPos * (1.0f - alpha) + pcR.maxPos * alpha);
                minV = std::min(minV, pfR.minNeg * (1.0f - alpha) + pcR.minNeg * alpha);
            }
            
            float px = (float)((s - startSample) * pixelsPerSample) + area.getX();
            float pyTop = clampY(midY - (maxV * halfH * 0.95f));
            float pyBottom = clampY(midY - (minV * halfH * 0.95f));

            // Agregación Sub-píxel: solo dibujamos cuando px ha avanzado aproximadamente 1 píxel,
            // pero manteniendo la posición FLOAT exacta para evitar el jitter.
            if (px >= lastPx + 0.95f) { 
                if (hasPending && lastPx >= area.getX() && lastPx < area.getRight()) {
                    float width = std::max(1.0f, px - lastPx);
                    g.fillRect(lastPx, currentY0, width, currentY1 - currentY0);
                    
                    if (spp < 64.0) {
                        g.setColour(color.withAlpha(0.3f));
                        g.fillRect(lastPx, currentY0 - 1.0f, width, 2.0f);
                        g.fillRect(lastPx, currentY1 - 1.0f, width, 2.0f);
                        g.setColour(color.withAlpha(1.0f));
                    }
                }
                lastPx = px;
                currentY0 = pyTop;
                currentY1 = pyBottom;
                hasPending = true;
            } else {
                currentY0 = std::min(currentY0, pyTop);
                currentY1 = std::max(currentY1, pyBottom);
            }
        }
        
        if (hasPending && lastPx >= area.getX() && lastPx < area.getRight()) {
            g.fillRect(lastPx, currentY0, 1.0f, currentY1 - currentY0);
        }
    } else {
        // --- MOTOR ORIGINAL (SIN TOCAR) ---
        for (long long i = idxStart; i < idxEnd; ++i) {
            double s = (double)i * sppLOD;
            AudioPeak pFine = getBlendedPeak(clip, s, levelFine, mode, chanIdx);
            AudioPeak pCoarse = getBlendedPeak(clip, s, levelCoarse, mode, chanIdx);
            float maxV = pFine.maxPos * (1.0f - alpha) + pCoarse.maxPos * alpha;
            float minV = pFine.minNeg * (1.0f - alpha) + pCoarse.minNeg * alpha;

            if (isStereo && mode == WaveformViewMode::Combined) {
                AudioPeak pfR = getBlendedPeak(clip, s, levelFine, mode, 1);
                AudioPeak pcR = getBlendedPeak(clip, s, levelCoarse, mode, 1);
                maxV = std::max(maxV, pfR.maxPos * (1.0f - alpha) + pcR.maxPos * alpha);
                minV = std::min(minV, pfR.minNeg * (1.0f - alpha) + pcR.minNeg * alpha);
            }
            
            float px = (float)((s - startSample) * pixelsPerSample) + area.getX();
            float pyTop = clampY(midY - (maxV * halfH * 0.95f));
            float pyBottom = clampY(midY - (minV * halfH * 0.95f));
            
            if (px >= area.getX() && px < area.getRight()) {
                g.fillRect(px, pyTop, 1.0f, pyBottom - pyTop);
                if (spp < 64.0) { // Borde estético
                    g.setColour(color.withAlpha(0.3f));
                    g.fillRect(px, pyTop - 1.0f, 1.0f, 2.0f);
                    g.fillRect(px, pyBottom - 1.0f, 1.0f, 2.0f);
                    g.setColour(color.withAlpha(1.0f));
                }
            }
        }
    }
}

void AudioClipRenderer::drawSpectrogram(juce::Graphics& g, const AudioClip& clip, juce::Rectangle<float> area, 
    double hZoom, double pps, double startSample, float baseW) 
{
    const auto& img = clip.getBuffer().getNumSamples() > 0 ? ((AudioClip&)clip).getSpectrogram() : juce::Image(); // Hack para evitar const
    if (img.isNull()) return;
    const double totalW = (double)baseW * hZoom;
    float imgPPP = (float)img.getWidth() / (float)totalW;
    int srcX = (int)std::floor(startSample * (pps * imgPPP));
    int srcW = (int)std::ceil((double)area.getWidth() * (pps * imgPPP));
    if (srcW > 0 && srcX >= 0)
        g.drawImage(img, area.getX(), area.getY(), area.getWidth(), area.getHeight(), srcX, 0, srcW, img.getHeight());
}

void AudioClipRenderer::drawWaveformSummary(juce::Graphics& g, const AudioClip& clip, juce::Rectangle<float> area, 
    double hZoom, double clipStartUnits, double viewStartUnits) 
{
    if (clip.getPeaksL(2).empty() || hZoom <= 0.0) return;
    float baseW = clip.getOriginalWidth() <= 0 ? clip.getWidth() : clip.getOriginalWidth();
    const double samplesPerUnit = (double)clip.getBuffer().getNumSamples() / (double)baseW;
    const double samplesPerPixel = samplesPerUnit / hZoom;
    const double pixelsPerSample = 1.0 / std::max(1e-7, samplesPerPixel);
    const double startSample = (viewStartUnits - clipStartUnits + clip.getOffsetX()) * samplesPerUnit;
    g.setOpacity(0.8f);
    renderSeamlessLayer(g, clip, area, startSample, startSample + area.getWidth() * samplesPerPixel, pixelsPerSample, samplesPerPixel, WaveformViewMode::Combined, 0, juce::Colours::white);
}
