/*
  ==============================================================================
    PolarPatterns.h
    Visualizador estilo Ozone "Solid Polar" - FULL SIZE & FULL WIDTH.

    AJUSTE FINAL:
    - Se elimina el margen de seguridad del radio (ahora usa el 100% del ancho).
    - Ganancia visual alta para que la señal llene el espacio.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>

class PolarVectorscope
{
public:
    PolarVectorscope()
    {
        bins.resize(180, 0.0f);
    }

    void process(const float* bufferL, const float* bufferR, int numSamples)
    {
        const float decay = 0.85f;

        for (auto& b : bins) b *= decay;

        for (int i = 0; i < numSamples; i += 2)
        {
            float l = bufferL[i];
            float r = bufferR[i];

            // 1. Mid/Side
            float mid = (l + r) * 0.707f;
            float side = (r - l) * 0.707f;

            float magVector = std::sqrt(mid * mid + side * side);
            if (magVector < 0.001f) continue;

            if (mid < 0) { mid = -mid; side = -side; }

            float angle = std::atan2(side, mid);
            angle *= 2.0f;

            float normalizedPos = (angle / juce::MathConstants<float>::pi + 0.5f);
            float exactPos = normalizedPos * (bins.size() - 1);
            exactPos = juce::jlimit(0.0f, (float)bins.size() - 1.001f, exactPos);

            // Ganancia fuerte
            float visualMag = magVector * 2.4f;

            int idx1 = (int)exactPos;
            int idx2 = idx1 + 1;
            float frac = exactPos - idx1;

            if (visualMag > bins[idx1])
                bins[idx1] = bins[idx1] * 0.5f + (visualMag * (1.0f - frac)) * 0.5f;

            if (idx2 < (int)bins.size()) {
                if (visualMag > bins[idx2])
                    bins[idx2] = bins[idx2] * 0.5f + (visualMag * frac) * 0.5f;
            }
        }

        smoothBins();
    }

    void draw(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour color)
    {
        if (bins.empty()) return;

        juce::Path path;
        float w = (float)bounds.getWidth();
        float h = (float)bounds.getHeight();
        float centerX = bounds.getCentreX();
        float bottomY = bounds.getBottom(); // Usamos hasta el último pixel

        // RADIO MÁXIMO: 100% del espacio disponible (sin márgenes 0.95)
        float maxRadius = std::min(w * 0.5f, h);

        path.startNewSubPath(centerX, bottomY);

        for (size_t i = 0; i < bins.size(); ++i)
        {
            float t = (float)i / (float)(bins.size() - 1);
            float angle = (t - 0.5f) * juce::MathConstants<float>::pi;

            float val = std::min(bins[i], 1.0f);
            float drawRadius = val * maxRadius;

            float drawX = centerX + (std::sin(angle) * drawRadius);
            float drawY = bottomY - (std::cos(angle) * drawRadius);

            path.lineTo(drawX, drawY);
        }

        path.lineTo(centerX, bottomY);
        path.closeSubPath();

        juce::ColourGradient grad(
            color.withAlpha(0.9f), centerX, bottomY,
            color.withAlpha(0.0f), centerX, bottomY - maxRadius,
            false
        );
        g.setGradientFill(grad);

        auto roundedPath = path.createPathWithRoundedCorners(2.0f);
        g.fillPath(roundedPath);

        g.setColour(color);
        g.strokePath(roundedPath, juce::PathStrokeType(1.0f));
    }

private:
    std::vector<float> bins;

    void smoothBins()
    {
        std::vector<float> temp = bins;
        for (size_t i = 1; i < bins.size() - 1; ++i)
        {
            bins[i] = temp[i - 1] * 0.2f + temp[i] * 0.6f + temp[i + 1] * 0.2f;
        }
    }
};