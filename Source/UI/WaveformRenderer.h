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

        if (cacheL.empty() || width <= 0 || height <= 0)
            return;

        const juce::Colour outlineColor = baseColor.darker(0.4f).withAlpha(1.0f);
        const juce::Colour fillColor = baseColor.brighter(0.1f).withAlpha(1.0f);
        
        float baseW = clipData.originalWidth <= 0 ? clipData.width : clipData.originalWidth;
        const float originalWidthPx = baseW * (float)hZoom;
        const float pointsPerPixel = originalWidthPx > 0 ? ((float)cacheL.size() / originalWidthPx) : 1.0f;
        const int cacheOffset = (int)(clipData.offsetX * (float)hZoom * pointsPerPixel);

        bool isStereo = !cacheR.empty();

        // --- SPECTROGRAM VIEW ---
        if (viewMode == WaveformViewMode::Spectrogram) {
            if (!clipData.spectrogramImage.isNull()) {
                 float imgPointsPerPixel = originalWidthPx > 0 ? ((float)clipData.spectrogramImage.getWidth() / originalWidthPx) : 1.0f;
                 int srcX = (int)(clipData.offsetX * (float)hZoom * imgPointsPerPixel);
                 int srcW = (int)(width * imgPointsPerPixel);
                 
                 if (srcW > 0 && srcX >= 0) {
                     g.drawImage(clipData.spectrogramImage, 
                                 area.getX(), area.getY(), area.getWidth(), area.getHeight(),
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
            if (startIdx == endIdx) {
                if (startIdx >= 0 && startIdx < (int)cache.size()) {
                    float frac = startIdxF - startIdx;
                    AudioPeak p1 = cache[startIdx];
                    AudioPeak p2 = (startIdx + 1 < (int)cache.size()) ? cache[startIdx + 1] : p1;
                    peak.maxPos = p1.maxPos + frac * (p2.maxPos - p1.maxPos);
                    peak.minNeg = p1.minNeg + frac * (p2.minNeg - p1.minNeg);
                }
            } else {
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
            }
            return peak;
        };

        // --- VISTA ESTÉREO (L/R) ---
        if (viewMode == WaveformViewMode::SeparateLR && isStereo) {
            const float halfHeight = (float)height / 2.0f;
            const float quarterHeight = halfHeight / 2.0f;

            const float midYL = (float)area.getY() + quarterHeight;
            const float midYR = (float)area.getY() + halfHeight + quarterHeight;

            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.drawHorizontalLine((int)(area.getY() + halfHeight), (float)area.getX(), (float)(area.getX() + width));

            for (int x = 0; x < width; ++x) {
                AudioPeak peakL = getPeakRange(cacheL, x);
                AudioPeak peakR = getPeakRange(cacheR, x);
                const int currentX = area.getX() + x;

                float topYL = midYL - (juce::jmin(1.0f, peakL.maxPos * 1.05f) * quarterHeight);
                float bottomYL = midYL - (juce::jmax(-1.0f, peakL.minNeg * 1.05f) * quarterHeight);

                g.setColour(fillColor);
                g.drawVerticalLine(currentX, topYL, bottomYL);
                g.setColour(outlineColor);
                g.fillRect((float)currentX, topYL, 1.0f, 1.0f);
                g.fillRect((float)currentX, bottomYL, 1.0f, 1.0f);

                float topYR = midYR - (juce::jmin(1.0f, peakR.maxPos * 1.05f) * quarterHeight);
                float bottomYR = midYR - (juce::jmax(-1.0f, peakR.minNeg * 1.05f) * quarterHeight);

                g.setColour(fillColor);
                g.drawVerticalLine(currentX, topYR, bottomYR);
                g.setColour(outlineColor);
                g.fillRect((float)currentX, topYR, 1.0f, 1.0f);
                g.fillRect((float)currentX, bottomYR, 1.0f, 1.0f);
            }
        }
        // --- NUEVA VISTA: MID / SIDE ---
        else if (viewMode == WaveformViewMode::MidSide && isStereo) {
            const float halfHeight = (float)height / 2.0f;
            const float quarterHeight = halfHeight / 2.0f;

            const float midYL = (float)area.getY() + quarterHeight;
            const float midYR = (float)area.getY() + halfHeight + quarterHeight;

            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.drawHorizontalLine((int)(area.getY() + halfHeight), (float)area.getX(), (float)(area.getX() + width));

            for (int x = 0; x < width; ++x) {
                AudioPeak peakMid = getPeakRange(cacheMid, x);
                AudioPeak peakSide = getPeakRange(cacheSide, x);
                const int currentX = area.getX() + x;

                float topYMid = midYL - (juce::jmin(1.0f, peakMid.maxPos * 1.05f) * quarterHeight);
                float bottomYMid = midYL - (juce::jmax(-1.0f, peakMid.minNeg * 1.05f) * quarterHeight);

                g.setColour(fillColor);
                g.drawVerticalLine(currentX, topYMid, bottomYMid);
                g.setColour(outlineColor);
                g.fillRect((float)currentX, topYMid, 1.0f, 1.0f);
                g.fillRect((float)currentX, bottomYMid, 1.0f, 1.0f);

                float topYSide = midYR - (juce::jmin(1.0f, peakSide.maxPos * 1.05f) * quarterHeight);
                float bottomYSide = midYR - (juce::jmax(-1.0f, peakSide.minNeg * 1.05f) * quarterHeight);

                g.setColour(fillColor);
                g.drawVerticalLine(currentX, topYSide, bottomYSide);
                g.setColour(outlineColor);
                g.fillRect((float)currentX, topYSide, 1.0f, 1.0f);
                g.fillRect((float)currentX, bottomYSide, 1.0f, 1.0f);
            }
        }
        // --- VISTA CLÁSICA (Combinada + Side interno) ---
        else {
            const float midY = (float)area.getY() + (float)height / 2.0f;
            const float halfHeight = (float)height / 2.0f;

            for (int x = 0; x < width; ++x) {
                AudioPeak peakL = getPeakRange(cacheL, x);
                AudioPeak peak = peakL;
                if (!cacheR.empty()) {
                    AudioPeak peakR = getPeakRange(cacheR, x);
                    peak.maxPos = std::max(peakL.maxPos, peakR.maxPos);
                    peak.minNeg = std::min(peakL.minNeg, peakR.minNeg);
                }

                peak.maxPos = juce::jmin(1.0f, peak.maxPos * 1.05f);
                peak.minNeg = juce::jmax(-1.0f, peak.minNeg * 1.05f);

                const int currentX = area.getX() + x;

                float topY = midY - (peak.maxPos * halfHeight);
                float bottomY = midY - (peak.minNeg * halfHeight);

                g.setColour(fillColor);
                g.drawVerticalLine(currentX, topY, bottomY);
                g.setColour(outlineColor);
                g.fillRect((float)currentX, topY, 1.0f, 1.0f);
                g.fillRect((float)currentX, bottomY, 1.0f, 1.0f);

                // Side superpuesto como marca outline flotante
                if (!cacheSide.empty()) {
                     AudioPeak pSide = getPeakRange(cacheSide, x);
                     float sideTop = midY - (juce::jmin(1.0f, pSide.maxPos * 1.05f) * halfHeight * 0.9f);
                     float sideBot = midY - (juce::jmax(-1.0f, pSide.minNeg * 1.05f) * halfHeight * 0.9f);
                     g.setColour(fillColor);
                     g.drawVerticalLine(currentX, sideTop, sideBot);
                     g.setColour(outlineColor);
                     g.fillRect((float)currentX, sideTop, 1.0f, 1.0f);
                     g.fillRect((float)currentX, sideBot, 1.0f, 1.0f);
                }
            }
        }
    }
};