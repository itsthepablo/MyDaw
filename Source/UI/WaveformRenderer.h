#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h" 
#include <vector>
#include <cmath>
#include <algorithm>

class WaveformRenderer {
public:
    static void drawWaveform(juce::Graphics& g,
        const AudioClipData& clipData,
        juce::Rectangle<int> area,
        juce::Colour baseColor,
        WaveformViewMode viewMode,
        double hZoom = 1.0)
    {
        const auto& cacheL = clipData.cachedPeaksL;
        const auto& cacheR = clipData.cachedPeaksR;
        const auto& cacheMid = clipData.cachedPeaksMid;
        const auto& cacheSide = clipData.cachedPeaksSide;

        const int width = area.getWidth();
        const int height = area.getHeight();

        if (!clipData.isLoaded.load(std::memory_order_relaxed)) {
            g.setColour(baseColor.brighter(0.5f));
            g.setFont(14.0f);
            g.drawText("Cargando...", area, juce::Justification::centred, true);
            return;
        }

        if (cacheL.empty() || width <= 0 || height <= 0)
            return;

        const juce::Colour outlineColor = baseColor.darker(0.4f).withAlpha(1.0f);
        const juce::Colour fillColor = baseColor.brighter(0.1f).withAlpha(1.0f);
        
        float baseW = clipData.originalWidth <= 0 ? clipData.width : clipData.originalWidth;
        const float originalWidthPx = baseW * (float)hZoom;
        const float pointsPerPixel = originalWidthPx > 0 ? ((float)cacheL.size() / originalWidthPx) : 1.0f;
        const int cacheOffset = (int)(clipData.offsetX * (float)hZoom * pointsPerPixel);

       const float samplesPerPixel = originalWidthPx > 0 ? ((float)clipData.fileBuffer.getNumSamples() / originalWidthPx) : 1.0f;
        const int sampleOffset = (int)(clipData.offsetX * (float)hZoom * samplesPerPixel);

        bool isStereo = !cacheR.empty();

        // --- VISIBILITY CULLING ---
        juce::Rectangle<int> clipBounds = g.getClipBounds();
        int drawStartX = std::max(0, clipBounds.getX() - area.getX());
        int drawEndX = std::min(width, clipBounds.getRight() - area.getX());

        if (drawStartX >= drawEndX) return;

        // --- MICRO MODE (SAMPLE-ACCURATE) ---
        if (samplesPerPixel <= 1.0f && clipData.fileBuffer.getNumSamples() > 0 && viewMode != WaveformViewMode::Spectrogram) {
            auto drawMicroChannel = [&](int channel, float offsetY, float renderHeight, juce::Colour color, bool isMid, bool isSide) {
                const float* readL = clipData.fileBuffer.getReadPointer(0);
                const float* readR = isStereo ? clipData.fileBuffer.getReadPointer(1) : nullptr;
                const int maxSamples = clipData.fileBuffer.getNumSamples();
                const float pixelsPerSample = 1.0f / samplesPerPixel;

                int startSample = std::max(0, sampleOffset + (int)(drawStartX * samplesPerPixel));
                int endSample = std::min(maxSamples - 1, sampleOffset + (int)(drawEndX * samplesPerPixel) + 1);

                juce::Path path;
                bool isFirst = true;

                for (int s = startSample; s <= endSample; ++s) {
                    float xPos = area.getX() + (s - sampleOffset) * pixelsPerSample;

                    float val = 0.0f;
                    if (isMid && isStereo) val = (readL[s] + readR[s]) * 0.5f;
                    else if (isSide && isStereo) val = (readL[s] - readR[s]) * 0.5f;
                    else if (channel == 0) val = readL[s];
                    else if (channel == 1 && isStereo) val = readR[s];
                    else if (channel == -1 && isStereo) val = std::abs(readL[s]) > std::abs(readR[s]) ? readL[s] : readR[s];

                    float yPos = offsetY - (juce::jlimit(-1.0f, 1.0f, val) * renderHeight);

                    if (isFirst) {
                        path.startNewSubPath(xPos, yPos);
                        isFirst = false;
                    } else {
                        path.lineTo(xPos, yPos);
                    }
                }

                g.setColour(color);
                g.strokePath(path, juce::PathStrokeType(1.5f));

                if (pixelsPerSample > 8.0f) {
                    g.setColour(outlineColor.brighter(0.5f));
                    for (int s = startSample; s <= endSample; ++s) {
                        float xPos = area.getX() + (s - sampleOffset) * pixelsPerSample;

                        float val = 0.0f;
                        if (isMid && isStereo) val = (readL[s] + readR[s]) * 0.5f;
                        else if (isSide && isStereo) val = (readL[s] - readR[s]) * 0.5f;
                        else if (channel == 0) val = readL[s];
                        else if (channel == 1 && isStereo) val = readR[s];
                        else if (channel == -1 && isStereo) val = std::abs(readL[s]) > std::abs(readR[s]) ? readL[s] : readR[s];

                        float yPos = offsetY - (juce::jlimit(-1.0f, 1.0f, val) * renderHeight);
                        g.fillEllipse(xPos - 3.0f, yPos - 3.0f, 6.0f, 6.0f);
                    }
                }
            };

            if (viewMode == WaveformViewMode::SeparateLR && isStereo) {
                const float halfH = (float)height / 2.0f;
                const float qH = halfH / 2.0f;
                g.setColour(juce::Colours::black.withAlpha(0.2f));
                g.drawHorizontalLine((int)(area.getY() + halfH), (float)area.getX(), (float)(area.getX() + width));
                drawMicroChannel(0, area.getY() + qH, qH, fillColor, false, false);
                drawMicroChannel(1, area.getY() + halfH + qH, qH, fillColor, false, false);
            } else if (viewMode == WaveformViewMode::MidSide && isStereo) {
                const float halfH = (float)height / 2.0f;
                const float qH = halfH / 2.0f;
                g.setColour(juce::Colours::black.withAlpha(0.2f));
                g.drawHorizontalLine((int)(area.getY() + halfH), (float)area.getX(), (float)(area.getX() + width));
                drawMicroChannel(0, area.getY() + qH, qH, fillColor, true, false);
                drawMicroChannel(1, area.getY() + halfH + qH, qH, fillColor, false, true);
            } else {
                const float midY = area.getY() + (float)height / 2.0f;
                drawMicroChannel(isStereo ? -1 : 0, midY, (float)height / 2.0f, fillColor, false, false);
            }
            return;
        }

        // --- MID MODE (TRANSICIONAL < 256 / FILEBUFFER NATIVO) ---
        else if (samplesPerPixel <= 256.0f && clipData.fileBuffer.getNumSamples() > 0 && viewMode != WaveformViewMode::Spectrogram) {
            auto drawRectChannel = [&](int channel, float offsetY, float renderHeight, juce::Colour color, bool isMid, bool isSide) {
                const float* readL = clipData.fileBuffer.getReadPointer(0);
                const float* readR = isStereo ? clipData.fileBuffer.getReadPointer(1) : nullptr;
                const int maxSamples = clipData.fileBuffer.getNumSamples();

                g.setColour(color);

                for (int x = drawStartX; x < drawEndX; ++x) {
                    int startSample = std::max(0, sampleOffset + (int)(x * samplesPerPixel));
                    int endSample = std::min(maxSamples - 1, sampleOffset + (int)((x + 1) * samplesPerPixel));
                    
                    if (startSample > endSample) continue;

                    float pMax = -1.0f;
                    float pMin = 1.0f;

                    for (int s = startSample; s <= endSample; ++s) {
                        float val = 0.0f;
                        if (isMid && isStereo) val = (readL[s] + readR[s]) * 0.5f;
                        else if (isSide && isStereo) val = (readL[s] - readR[s]) * 0.5f;
                        else if (channel == 0) val = readL[s];
                        else if (channel == 1 && isStereo) val = readR[s];
                        else if (channel == -1 && isStereo) val = std::abs(readL[s]) > std::abs(readR[s]) ? readL[s] : readR[s];
                        
                        pMax = std::max(pMax, val);
                        pMin = std::min(pMin, val);
                    }

                    if (pMax < pMin) continue;

                    float topY = offsetY - (juce::jlimit(-1.0f, 1.0f, pMax) * renderHeight);
                    float bottomY = offsetY - (juce::jlimit(-1.0f, 1.0f, pMin) * renderHeight);

                    g.fillRect((float)(area.getX() + x), topY, 1.0f, std::max(1.0f, bottomY - topY));
                }
            };

            if (viewMode == WaveformViewMode::SeparateLR && isStereo) {
                const float halfH = (float)height / 2.0f;
                const float qH = halfH / 2.0f;
                g.setColour(juce::Colours::black.withAlpha(0.2f));
                g.drawHorizontalLine((int)(area.getY() + halfH), (float)area.getX(), (float)(area.getX() + width));
                drawRectChannel(0, area.getY() + qH, qH, fillColor, false, false);
                drawRectChannel(1, area.getY() + halfH + qH, qH, fillColor, false, false);
            } else if (viewMode == WaveformViewMode::MidSide && isStereo) {
                const float halfH = (float)height / 2.0f;
                const float qH = halfH / 2.0f;
                g.setColour(juce::Colours::black.withAlpha(0.2f));
                g.drawHorizontalLine((int)(area.getY() + halfH), (float)area.getX(), (float)(area.getX() + width));
                drawRectChannel(0, area.getY() + qH, qH, fillColor, true, false);
                drawRectChannel(1, area.getY() + halfH + qH, qH, fillColor, false, true);
            } else {
                const float midY = area.getY() + (float)height / 2.0f;
                drawRectChannel(isStereo ? -1 : 0, midY, (float)height / 2.0f, fillColor, false, false);
            }
            return;
        }

        // --- SPECTROGRAM VIEW ---
        if (viewMode == WaveformViewMode::Spectrogram) {
            if (!clipData.spectrogramImage.isNull()) {
                 float imgPointsPerPixel = originalWidthPx > 0 ? ((float)clipData.spectrogramImage.getWidth() / originalWidthPx) : 1.0f;
                 int srcX = (int)((clipData.offsetX + drawStartX) * (float)hZoom * imgPointsPerPixel);
                 int srcW = (int)((drawEndX - drawStartX) * imgPointsPerPixel);
                 
                 if (srcW > 0 && srcX >= 0) {
                     g.drawImage(clipData.spectrogramImage, 
                                 area.getX() + drawStartX, area.getY(), drawEndX - drawStartX, area.getHeight(),
                                 srcX, 0, srcW, clipData.spectrogramImage.getHeight());
                 }
            }
            return;
        }

        auto getPeakRange = [&](const std::vector<AudioPeak>& cache, int x) -> AudioPeak {
            float startIdxF = cacheOffset + x * pointsPerPixel;
            float endIdxF = startIdxF + pointsPerPixel;
            int startIdx = (int)startIdxF;
            int endIdx = (int)endIdxF;

            AudioPeak peak;
            int safeStart = std::max(0, startIdx);
            int safeEnd = std::min((int)cache.size() - 1, endIdx);
            if (safeStart <= safeEnd) {
                peak.maxPos = cache[safeStart].maxPos;
                peak.minNeg = cache[safeStart].minNeg;
                for (int i = safeStart + 1; i <= safeEnd; ++i) {
                    peak.maxPos = std::max(peak.maxPos, cache[i].maxPos);
                    peak.minNeg = std::min(peak.minNeg, cache[i].minNeg);
                }
            }
            return peak;
        };

        // --- MACRO MODE (VISTA ESTÉREO L/R) ---
        if (viewMode == WaveformViewMode::SeparateLR && isStereo) {
            const float halfHeight = (float)height / 2.0f;
            const float quarterHeight = halfHeight / 2.0f;

            const float midYL = (float)area.getY() + quarterHeight;
            const float midYR = (float)area.getY() + halfHeight + quarterHeight;

            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.drawHorizontalLine((int)(area.getY() + halfHeight), (float)area.getX(), (float)(area.getX() + width));

            for (int x = drawStartX; x < drawEndX; ++x) {
                AudioPeak peakL = getPeakRange(cacheL, x);
                AudioPeak peakR = getPeakRange(cacheR, x);
                const int currentX = area.getX() + x;

                float topYL = midYL - (juce::jlimit(-1.0f, 1.0f, peakL.maxPos) * quarterHeight);
                float bottomYL = midYL - (juce::jlimit(-1.0f, 1.0f, peakL.minNeg) * quarterHeight);

                g.setColour(fillColor);
                g.fillRect((float)currentX, topYL, 1.0f, std::max(1.0f, bottomYL - topYL));

                float topYR = midYR - (juce::jlimit(-1.0f, 1.0f, peakR.maxPos) * quarterHeight);
                float bottomYR = midYR - (juce::jlimit(-1.0f, 1.0f, peakR.minNeg) * quarterHeight);

                g.fillRect((float)currentX, topYR, 1.0f, std::max(1.0f, bottomYR - topYR));
            }
        }
        // --- MACRO MODE (MID / SIDE) ---
        else if (viewMode == WaveformViewMode::MidSide && isStereo) {
            const float halfHeight = (float)height / 2.0f;
            const float quarterHeight = halfHeight / 2.0f;

            const float midYL = (float)area.getY() + quarterHeight;
            const float midYR = (float)area.getY() + halfHeight + quarterHeight;

            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.drawHorizontalLine((int)(area.getY() + halfHeight), (float)area.getX(), (float)(area.getX() + width));

            for (int x = drawStartX; x < drawEndX; ++x) {
                AudioPeak peakMid = getPeakRange(cacheMid, x);
                AudioPeak peakSide = getPeakRange(cacheSide, x);
                const int currentX = area.getX() + x;

                float topYMid = midYL - (juce::jlimit(-1.0f, 1.0f, peakMid.maxPos) * quarterHeight);
                float bottomYMid = midYL - (juce::jlimit(-1.0f, 1.0f, peakMid.minNeg) * quarterHeight);

                g.setColour(fillColor);
                g.fillRect((float)currentX, topYMid, 1.0f, std::max(1.0f, bottomYMid - topYMid));

                float topYSide = midYR - (juce::jlimit(-1.0f, 1.0f, peakSide.maxPos) * quarterHeight);
                float bottomYSide = midYR - (juce::jlimit(-1.0f, 1.0f, peakSide.minNeg) * quarterHeight);

                g.fillRect((float)currentX, topYSide, 1.0f, std::max(1.0f, bottomYSide - topYSide));
            }
        }
        // --- MACRO MODE (CLÁSICA COMBINADA) ---
        else {
            const float midY = (float)area.getY() + (float)height / 2.0f;
            const float halfHeight = (float)height / 2.0f;

            for (int x = drawStartX; x < drawEndX; ++x) {
                AudioPeak peakL = getPeakRange(cacheL, x);
                AudioPeak peak = peakL;
                if (!cacheR.empty()) {
                    AudioPeak peakR = getPeakRange(cacheR, x);
                    peak.maxPos = std::max(peakL.maxPos, peakR.maxPos);
                    peak.minNeg = std::min(peakL.minNeg, peakR.minNeg);
                }

                float topY = midY - (juce::jlimit(-1.0f, 1.0f, peak.maxPos) * halfHeight);
                float bottomY = midY - (juce::jlimit(-1.0f, 1.0f, peak.minNeg) * halfHeight);

                g.setColour(fillColor);
                g.fillRect((float)(area.getX() + x), topY, 1.0f, std::max(1.0f, bottomY - topY));
            }
        }
    }

    // ====================================================================================
    // RENDERIZADO DE RESUMEN (FANTASMA) PARA CARPETAS
    // ====================================================================================
    static void drawWaveformSummary(juce::Graphics& g,
        const AudioClipData& clipData,
        juce::Rectangle<int> area,
        juce::Colour color,
        double hZoom)
    {
        const auto& cache = !clipData.cachedPeaksMid.empty() ? clipData.cachedPeaksMid : clipData.cachedPeaksL;
        if (cache.empty()) return;

        const int width = area.getWidth();
        const int height = area.getHeight();
        if (width <= 0 || height <= 0) return;

        juce::Rectangle<int> clipBounds = g.getClipBounds();
        int drawStartX = std::max(0, clipBounds.getX() - area.getX());
        int drawEndX = std::min(width, clipBounds.getRight() - area.getX());

        if (drawStartX >= drawEndX) return;

        float baseW = clipData.originalWidth <= 0 ? clipData.width : clipData.originalWidth;
        const float originalWidthPx = baseW * (float)hZoom;
        const float pointsPerPixel = originalWidthPx > 0 ? ((float)cache.size() / originalWidthPx) : 1.0f;
        const int cacheOffset = (int)(clipData.offsetX * (float)hZoom * pointsPerPixel);

        const float midY = (float)area.getY() + (float)height / 2.0f;
        const float halfHeight = (float)height * 0.45f; 

        // COLOR BLANCO CON 80% DE OPACIDAD (como pediste para máxima visibilidad)
        g.setColour(juce::Colours::white.withAlpha(0.8f)); 

        for (int x = drawStartX; x < drawEndX; ++x) {
            int startIdx = (int)(cacheOffset + x * pointsPerPixel);
            int endIdx = (int)(cacheOffset + (x + 1) * pointsPerPixel);
            
            AudioPeak peak;
            int safeStart = std::max(0, startIdx);
            int safeEnd = std::min((int)cache.size() - 1, endIdx);
            
            if (safeStart < (int)cache.size()) {
                peak.maxPos = cache[safeStart].maxPos;
                peak.minNeg = cache[safeStart].minNeg;
                for (int i = safeStart + 1; i <= safeEnd; ++i) {
                    peak.maxPos = std::max(peak.maxPos, cache[i].maxPos);
                    peak.minNeg = std::min(peak.minNeg, cache[i].minNeg);
                }
            }

            float topY = midY - (juce::jlimit(0.0f, 1.0f, peak.maxPos) * halfHeight);
            float bottomY = midY - (juce::jlimit(-1.0f, 0.0f, peak.minNeg) * halfHeight);

            g.drawVerticalLine(area.getX() + x, topY, bottomY);
        }
    }
};