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

        // Lambda ultra rápido para calcular el Peak Sub-Pixel preciso y hacer interpolación en Zooms Cercanos
        auto getPeakRange = [&](const std::vector<float>& cache, int x) -> float {
            float startIdxF = cacheOffset + x * pointsPerPixel;
            float endIdxF = startIdxF + pointsPerPixel;
            int startIdx = (int)startIdxF;
            int endIdx = (int)endIdxF;

            float peak = 0.0f;
            if (startIdx == endIdx) {
                if (startIdx >= 0 && startIdx < (int)cache.size()) {
                    float frac = startIdxF - startIdx;
                    float p1 = cache[startIdx];
                    float p2 = (startIdx + 1 < (int)cache.size()) ? cache[startIdx + 1] : p1;
                    peak = p1 + frac * (p2 - p1);
                }
            } else {
                int safeStart = std::max(0, startIdx);
                int safeEnd = std::min((int)cache.size() - 1, endIdx);
                for (int i = safeStart; i <= safeEnd; ++i) {
                    peak = std::max(peak, cache[i]);
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
                float peakL = juce::jmin(1.0f, getPeakRange(cacheL, x) * 1.05f);
                float peakR = juce::jmin(1.0f, getPeakRange(cacheR, x) * 1.05f);
                const int currentX = area.getX() + x;

                float yOffsetL = peakL * quarterHeight;
                float topYL = midYL - yOffsetL;
                float bottomYL = midYL + yOffsetL;

                g.setColour(fillColor);
                g.drawVerticalLine(currentX, topYL, bottomYL);
                g.setColour(outlineColor);
                g.fillRect((float)currentX, topYL, 1.0f, 1.0f);
                g.fillRect((float)currentX, bottomYL, 1.0f, 1.0f);

                float yOffsetR = peakR * quarterHeight;
                float topYR = midYR - yOffsetR;
                float bottomYR = midYR + yOffsetR;

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
                float peakMid = juce::jmin(1.0f, getPeakRange(cacheMid, x) * 1.05f);
                float peakSide = juce::jmin(1.0f, getPeakRange(cacheSide, x) * 1.05f);
                const int currentX = area.getX() + x;

                // Mid
                float yOffsetMid = peakMid * quarterHeight;
                float topYMid = midYL - yOffsetMid;
                float bottomYMid = midYL + yOffsetMid;

                g.setColour(fillColor);
                g.drawVerticalLine(currentX, topYMid, bottomYMid);
                g.setColour(outlineColor);
                g.fillRect((float)currentX, topYMid, 1.0f, 1.0f);
                g.fillRect((float)currentX, bottomYMid, 1.0f, 1.0f);

                // Side
                float yOffsetSide = peakSide * quarterHeight;
                float topYSide = midYR - yOffsetSide;
                float bottomYSide = midYR + yOffsetSide;

                g.setColour(baseColor.darker(0.2f).withAlpha(1.0f));
                g.drawVerticalLine(currentX, topYSide, bottomYSide);
                g.setColour(outlineColor);
                g.fillRect((float)currentX, topYSide, 1.0f, 1.0f);
                g.fillRect((float)currentX, bottomYSide, 1.0f, 1.0f);
            }
        }
        // --- VISTA CLÁSICA (Combinada) ---
        else {
            const float midY = (float)area.getY() + (float)height / 2.0f;
            const float halfHeight = (float)height / 2.0f;

            for (int x = 0; x < width; ++x) {
                float peakL = getPeakRange(cacheL, x);
                float peak = peakL;
                if (!cacheR.empty()) {
                    float peakR = getPeakRange(cacheR, x);
                    peak = std::max(peakL, peakR);
                }

                peak = juce::jmin(1.0f, peak * 1.05f);
                const int currentX = area.getX() + x;

                float yOffset = peak * halfHeight;
                float topY = midY - yOffset;
                float bottomY = midY + yOffset;

                g.setColour(fillColor);
                g.drawVerticalLine(currentX, topY, bottomY);
                g.setColour(outlineColor);
                g.fillRect((float)currentX, topY, 1.0f, 1.0f);
                g.fillRect((float)currentX, bottomY, 1.0f, 1.0f);
            }
        }
    }
};