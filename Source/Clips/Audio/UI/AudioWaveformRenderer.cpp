#include "AudioWaveformRenderer.h"
#include <cmath>
#include <vector>
#include <algorithm>

void AudioWaveformRenderer::renderWaveformMode(juce::Graphics& g, 
                                             const AudioClip& clip, 
                                             juce::Rectangle<float> area,
                                             WaveformViewMode mode,
                                             double visibleStartS,
                                             double visibleEndS,
                                             double pixelsPerSample,
                                             double samplesPerPixel,
                                             juce::Colour waveColor)
{
    g.setColour(waveColor);
    
    if (mode == WaveformViewMode::SeparateLR && clip.getBuffer().getNumChannels() > 1) {
        float h = area.getHeight() / 2.0f;
        renderSeamlessLayer(g, clip, area.withHeight(h), visibleStartS, visibleEndS, pixelsPerSample, samplesPerPixel, WaveformViewMode::SeparateLR, 0, waveColor);
        renderSeamlessLayer(g, clip, area.withY(area.getY() + h).withHeight(h), visibleStartS, visibleEndS, pixelsPerSample, samplesPerPixel, WaveformViewMode::SeparateLR, 1, waveColor);
    } else if (mode == WaveformViewMode::MidSide && clip.getBuffer().getNumChannels() > 1) {
        float h = area.getHeight() / 2.0f;
        renderSeamlessLayer(g, clip, area.withHeight(h), visibleStartS, visibleEndS, pixelsPerSample, samplesPerPixel, WaveformViewMode::MidSide, 0, waveColor);
        renderSeamlessLayer(g, clip, area.withY(area.getY() + h).withHeight(h), visibleStartS, visibleEndS, pixelsPerSample, samplesPerPixel, WaveformViewMode::MidSide, 1, waveColor);
    } else {
        renderSeamlessLayer(g, clip, area, visibleStartS, visibleEndS, pixelsPerSample, samplesPerPixel, mode, 0, waveColor);
    }
}

double AudioWaveformRenderer::calculateLODLevel(double spp) {
    if (spp <= 1.0) return -1.0; 
    if (spp <= 256.0) return (std::log(spp) / std::log(256.0)) - 1.0; 
    return std::log(spp / 256.0) / std::log(4.0);
}

AudioPeak AudioWaveformRenderer::getBlendedPeak(const AudioClip& clip, double s, int level, WaveformViewMode mode, int chanIdx) {
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

void AudioWaveformRenderer::renderSeamlessLayer(juce::Graphics& g, const AudioClip& clip, juce::Rectangle<float> area,
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
        // Renderizado Micro (Lineas entre muestras)
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

    if (idxEnd <= idxStart) return;

    // --- MOTOR VECTORIAL UNIFICADO (MAXIMA CALIDAD, CERO JITTER, CERO PERSIANAS) ---
    // Usamos juce::Path para que las primitivas fluyan juntas sin solapamiento (Moire)
    if (spp >= 2.0) {
        std::vector<juce::Point<float>> tops;
        std::vector<juce::Point<float>> bottoms;
        tops.reserve((size_t)(idxEnd - idxStart + 2));
        bottoms.reserve((size_t)(idxEnd - idxStart + 2));

        float lastPx = -1e9f;
        float currentY0 = midY;
        float currentY1 = midY;
        bool hasPending = false;
        
        // Logica de densidad restaurada a tu version original perfecta.
        float pxBase = (levelFine < 0) ? 0.3f : 0.0f;
        float pxThreshold = (levelFine < 0) ? std::min(pxBase, std::max(0.0f, (float)(spp - 2.0) / 14.0f * pxBase)) : 0.0f;

        for (long long i = idxStart; i < idxEnd; ++i) {
            double s = (double)i * sppLOD;
            AudioPeak pFine = getBlendedPeak(clip, s, levelFine, mode, chanIdx);
            AudioPeak pCoarse = getBlendedPeak(clip, s, levelCoarse, mode, chanIdx);
            
            float effectiveAlpha = alpha;
            if (levelFine < 0) effectiveAlpha *= std::min(1.0f, std::max(0.0f, (float)(spp - 2.0) / 14.0f));

            float maxV = pFine.maxPos * (1.0f - effectiveAlpha) + pCoarse.maxPos * effectiveAlpha;
            float minV = pFine.minNeg * (1.0f - effectiveAlpha) + pCoarse.minNeg * effectiveAlpha;

            if (isStereo && mode == WaveformViewMode::Combined) {
                AudioPeak pfR = getBlendedPeak(clip, s, levelFine, mode, 1);
                AudioPeak pcR = getBlendedPeak(clip, s, levelCoarse, mode, 1);
                maxV = std::max(maxV, pfR.maxPos * (1.0f - effectiveAlpha) + pcR.maxPos * effectiveAlpha);
                minV = std::min(minV, pfR.minNeg * (1.0f - effectiveAlpha) + pcR.minNeg * effectiveAlpha);
            }
            
            float px = (float)((s - startSample) * pixelsPerSample) + area.getX();
            float pyTop = clampY(midY - (maxV * halfH * 0.95f));
            float pyBottom = clampY(midY - (minV * halfH * 0.95f));
            
            if (px >= lastPx + pxThreshold) {
                if (hasPending && lastPx >= area.getX() - 5.0f && lastPx < area.getRight() + 5.0f) {
                    tops.push_back({lastPx, currentY0});
                    bottoms.push_back({lastPx, currentY1});
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

        if (hasPending && lastPx >= area.getX() - 5.0f && lastPx < area.getRight() + 5.0f) {
            tops.push_back({lastPx, currentY0});
            bottoms.push_back({lastPx, currentY1});
        }

        if (!tops.empty()) {
            juce::Path wavePath;
            wavePath.startNewSubPath(tops[0]);
            for (size_t i = 1; i < tops.size(); ++i) wavePath.lineTo(tops[i]);
            for (size_t i = bottoms.size(); i-- > 0;) wavePath.lineTo(bottoms[i]);
            wavePath.closeSubPath();
            
            g.setColour(color);
            WaveformStyle style = clip.getStyle();
            
            if (style == WaveformStyle::Silhouette) {
                g.strokePath(wavePath, juce::PathStrokeType(1.5f));
            } else if (style == WaveformStyle::VerticalGradient) {
                juce::ColourGradient grad(color, 0.0f, midY, color.withAlpha(0.2f), 0.0f, area.getY(), false);
                grad.addColour(0.5, color);
                grad.addColour(1.0, color.withAlpha(0.2f));
                g.setGradientFill(grad);
                g.fillPath(wavePath);
                g.setColour(color.withAlpha(0.6f));
                g.strokePath(wavePath, juce::PathStrokeType(1.0f));
            } else if (style == WaveformStyle::NeonGlow) {
                for (float i = 6.0f; i > 1.0f; i -= 1.0f) {
                    g.setColour(color.withAlpha(0.1f * (1.0f - (i / 7.0f))));
                    g.strokePath(wavePath, juce::PathStrokeType(i));
                }
                g.setColour(color);
                g.fillPath(wavePath);
                g.setColour(juce::Colours::white.withAlpha(0.4f)); 
                g.strokePath(wavePath, juce::PathStrokeType(1.0f));
            } else {
                g.fillPath(wavePath); 
                if (spp < 64.0) { 
                    float strokeAlpha = 0.3f;
                    if (spp < 6.0) {
                        float factor = 1.0f - std::max(0.0f, (float)(spp - 2.0) / 4.0f);
                        strokeAlpha = 0.3f + 0.7f * factor;
                    }
                    g.setColour(color.withAlpha(strokeAlpha));
                    g.strokePath(wavePath, juce::PathStrokeType(1.5f));
                }
            }
        }
    }

    // --- DIAGNOSTICO VISUAL ---
    if (AudioClip::isDebugInfoEnabled()) {
        g.setColour(juce::Colours::black.withAlpha(0.7f));
        g.fillRect(area.getX(), area.getY(), 110.0f, 58.0f);
        g.setColour(juce::Colours::orange);
        g.setFont(10.0f);
        float scale = (float)g.getInternalContext().getPhysicalPixelScaleFactor();
        g.drawText("Scale: " + juce::String(scale, 2), area.getX() + 5, area.getY() + 5, 100, 12, juce::Justification::left);
        g.drawText("SPP: " + juce::String(spp, 1), area.getX() + 5, area.getY() + 18, 100, 12, juce::Justification::left);
        g.drawText("LOD: " + juce::String(lodVal, 2), area.getX() + 5, area.getY() + 31, 100, 12, juce::Justification::left);
        g.drawText("Alpha: " + juce::String(alpha, 2), area.getX() + 5, area.getY() + 44, 100, 12, juce::Justification::left);
    }
}

void AudioWaveformRenderer::drawWaveformSummary(juce::Graphics& g, const AudioClip& clip, juce::Rectangle<float> area, 
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
