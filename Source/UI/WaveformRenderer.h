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
        WaveformViewMode viewMode)
    {
        const auto& cacheL = clipData.cachedPeaksL;
        const auto& cacheR = clipData.cachedPeaksR;
        const auto& cacheMid = clipData.cachedPeaksMid;
        const auto& cacheSide = clipData.cachedPeaksSide;

        const int width = area.getWidth();
        const int height = area.getHeight();

        if (cacheL.empty() || width <= 0 || height <= 0)
            return;

        const juce::Colour outlineColor = baseColor.brighter(0.2f).withAlpha(0.9f);
        const juce::Colour fillColor = baseColor.withAlpha(0.9f);
        const float pointsPerPixel = (float)cacheL.size() / (float)width;

        bool isStereo = !cacheR.empty();

        // --- VISTA ESTÉREO (L/R) ---
        if (viewMode == WaveformViewMode::SeparateLR && isStereo) {
            const float halfHeight = (float)height / 2.0f;
            const float quarterHeight = halfHeight / 2.0f;

            const float midYL = (float)area.getY() + quarterHeight;
            const float midYR = (float)area.getY() + halfHeight + quarterHeight;

            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.drawHorizontalLine((int)(area.getY() + halfHeight), (float)area.getX(), (float)(area.getX() + width));

            for (int x = 0; x < width; ++x) {
                int cacheIdx = (int)(x * pointsPerPixel);
                if (cacheIdx >= (int)cacheL.size()) break;

                float peakL = juce::jmin(1.0f, cacheL[cacheIdx] * 1.05f);
                float peakR = juce::jmin(1.0f, cacheR[cacheIdx] * 1.05f);
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

            // Línea separadora tenue
            g.setColour(juce::Colours::black.withAlpha(0.2f));
            g.drawHorizontalLine((int)(area.getY() + halfHeight), (float)area.getX(), (float)(area.getX() + width));

            for (int x = 0; x < width; ++x) {
                int cacheIdx = (int)(x * pointsPerPixel);
                if (cacheIdx >= (int)cacheMid.size()) break;

                float peakMid = juce::jmin(1.0f, cacheMid[cacheIdx] * 1.05f);
                float peakSide = juce::jmin(1.0f, cacheSide[cacheIdx] * 1.05f);
                const int currentX = area.getX() + x;

                // Mid (Mitad Superior)
                float yOffsetMid = peakMid * quarterHeight;
                float topYMid = midYL - yOffsetMid;
                float bottomYMid = midYL + yOffsetMid;

                g.setColour(fillColor);
                g.drawVerticalLine(currentX, topYMid, bottomYMid);
                g.setColour(outlineColor);
                g.fillRect((float)currentX, topYMid, 1.0f, 1.0f);
                g.fillRect((float)currentX, bottomYMid, 1.0f, 1.0f);

                // Side (Mitad Inferior)
                float yOffsetSide = peakSide * quarterHeight;
                float topYSide = midYR - yOffsetSide;
                float bottomYSide = midYR + yOffsetSide;

                // Aplicamos un color ligeramente más oscuro a la señal Side para diferenciar visualmente
                g.setColour(baseColor.withAlpha(0.7f));
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
                int cacheIdx = (int)(x * pointsPerPixel);
                if (cacheIdx >= (int)cacheL.size()) break;

                float peak = cacheL[cacheIdx];
                if (!cacheR.empty()) {
                    peak = std::max(peak, cacheR[cacheIdx]);
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