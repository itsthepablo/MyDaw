#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <algorithm>

// ==============================================================================
// 1. SCROLLING WAVEFORM (POLARIDAD REAL + DECAY SUAVE + SIN LÍNEA EN SILENCIO)
// ==============================================================================
class ScrollingWaveform
{
public:
    ScrollingWaveform() {
        visualBufferSize = 2048;
        minBuffer.resize(visualBufferSize, 0.0f);
        maxBuffer.resize(visualBufferSize, 0.0f);
        writePos = 0;
        tempMin = 0.0f;
        tempMax = 0.0f;
    }

    void modifyZoom(float wheelAmount)
    {
        float multiplier = 1.0f;
        if (wheelAmount > 0) multiplier = 0.9f;
        else if (wheelAmount < 0) multiplier = 1.1f;

        currentSamplesPerPixel *= multiplier;
        if (currentSamplesPerPixel < 16.0f) currentSamplesPerPixel = 16.0f;
        if (currentSamplesPerPixel > 1024.0f) currentSamplesPerPixel = 1024.0f;
    }

    void pushSamples(const float* inL, const float* inR, int numSamples)
    {
        const int samplesNeeded = (int)currentSamplesPerPixel;

        for (int i = 0; i < numSamples; ++i) {
            float mono = (inL[i] + inR[i]) * 0.5f;

            if (mono < tempMin) tempMin = mono;
            if (mono > tempMax) tempMax = mono;

            accumulatedSamples++;
            if (accumulatedSamples >= samplesNeeded) {
                minBuffer[writePos] = tempMin;
                maxBuffer[writePos] = tempMax;
                writePos = (writePos + 1) % visualBufferSize;

                // Aplicamos un decay rápido para suavizar cortes en lugar de saltar a 0
                tempMin *= 0.85f;
                tempMax *= 0.85f;

                if (std::abs(tempMin) < 0.001f) tempMin = 0.0f;
                if (std::abs(tempMax) < 0.001f) tempMax = 0.0f;

                accumulatedSamples = 0;
            }
        }
    }

    void draw(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour baseColor = juce::Colours::cyan)
    {
        juce::Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(bounds);

        float midY = (float)bounds.getCentreY();
        float heightScale = (float)bounds.getHeight() * 0.5f;
        int w = bounds.getWidth();

        int readIndex = writePos - 1;
        if (readIndex < 0) readIndex = visualBufferSize - 1;

        struct Slice { float x, top, bottom; bool isSilence; };
        std::vector<Slice> slices;
        slices.reserve(w);

        for (int x = w - 1; x >= 0; --x) {
            float minVal = minBuffer[readIndex];
            float maxVal = maxBuffer[readIndex];

            bool isSilence = (maxVal < 0.001f && minVal > -0.001f);

            float top = midY - (maxVal * heightScale);
            float bottom = midY - (minVal * heightScale);

            if (!isSilence && std::abs(top - bottom) < 1.0f) {
                top -= 0.5f;
                bottom += 0.5f;
            }

            slices.push_back({ (float)(bounds.getX() + x), top, bottom, isSilence });

            readIndex--;
            if (readIndex < 0) readIndex = visualBufferSize - 1;
        }

        juce::Path fillPath;
        if (!slices.empty()) {
            fillPath.startNewSubPath(slices[0].x, slices[0].top);
            for (size_t i = 1; i < slices.size(); ++i) {
                fillPath.lineTo(slices[i].x, slices[i].top);
            }
            for (int i = (int)slices.size() - 1; i >= 0; --i) {
                fillPath.lineTo(slices[i].x, slices[i].bottom);
            }
            fillPath.closeSubPath();
        }

        g.setColour(baseColor.withAlpha(0.35f));
        g.fillPath(fillPath);

        juce::Path edgesPath;

        bool wasSilence = true;
        for (size_t i = 0; i < slices.size(); ++i) {
            const auto& slice = slices[i];
            if (!slice.isSilence) {
                if (wasSilence) {
                    edgesPath.startNewSubPath(slice.x, slice.top);
                    if (i == slices.size() - 1 || slices[i + 1].isSilence) {
                        edgesPath.lineTo(slice.x + 0.5f, slice.top);
                    }
                }
                else {
                    edgesPath.lineTo(slice.x, slice.top);
                }
                wasSilence = false;
            }
            else {
                wasSilence = true;
            }
        }

        wasSilence = true;
        for (int i = (int)slices.size() - 1; i >= 0; --i) {
            const auto& slice = slices[i];
            if (!slice.isSilence) {
                if (wasSilence) {
                    edgesPath.startNewSubPath(slice.x, slice.bottom);
                    if (i == 0 || slices[i - 1].isSilence) {
                        edgesPath.lineTo(slice.x - 0.5f, slice.bottom);
                    }
                }
                else {
                    edgesPath.lineTo(slice.x, slice.bottom);
                }
                wasSilence = false;
            }
            else {
                wasSilence = true;
            }
        }

        g.setColour(baseColor.withMultipliedBrightness(1.3f));
        g.strokePath(edgesPath, juce::PathStrokeType(1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

private:
    std::vector<float> minBuffer;
    std::vector<float> maxBuffer;
    int visualBufferSize;
    int writePos;
    float currentSamplesPerPixel = 1024.0f;
    int accumulatedSamples = 0;
    float tempMin;
    float tempMax;
};

// ==============================================================================
// 2. OSCILOSCOPIO LEGACY (TU CÓDIGO ORIGINAL INTACTO)
// ==============================================================================
class LegacyOscilloscope
{
public:
    LegacyOscilloscope()
    {
        bufferSize = 96000;
        circularBufferL.resize(bufferSize, 0.0f);
        circularBufferR.resize(bufferSize, 0.0f);
        writePos = 0;

        workBufferSize = 4096;
        workBufferL.resize(workBufferSize, 0.0f);
        workBufferR.resize(workBufferSize, 0.0f);
    }

    void pushBuffer(const float* dataL, const float* dataR, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            circularBufferL[writePos] = dataL[i];
            circularBufferR[writePos] = dataR[i];
            writePos++;
            if (writePos >= bufferSize) writePos = 0;
        }
    }

    void draw(juce::Graphics& g, int x, int y, int width, int height, juce::Colour color)
    {
        juce::Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(x, y, width, height);

        int readStart = writePos - workBufferSize;
        if (readStart < 0) readStart += bufferSize;

        for (int i = 0; i < workBufferSize; ++i)
        {
            int idx = (readStart + i) % bufferSize;
            workBufferL[i] = circularBufferL[idx];
            workBufferR[i] = circularBufferR[idx];
        }

        int triggerOffset = 0;
        bool armed = false;
        bool triggerFound = false;
        int searchRange = workBufferSize / 2;

        const float upperThreshold = 0.005f;
        const float lowerThreshold = -0.005f;

        for (int i = 0; i < searchRange; ++i)
        {
            float s = workBufferL[i];

            if (!armed)
            {
                if (s < lowerThreshold) armed = true;
            }
            else
            {
                if (s >= upperThreshold)
                {
                    triggerOffset = i;
                    triggerFound = true;
                    break;
                }
            }
        }

        if (!triggerFound) triggerOffset = 0;

        int samplesAvailable = workBufferSize - triggerOffset;
        int samplesToDraw = std::min(samplesAvailable, width * 2);

        if (samplesToDraw < 4) return;

        juce::Path midPath;
        juce::Path sidePath;
        std::vector<juce::Point<float>> topPoints, botPoints;
        topPoints.reserve(samplesToDraw);
        botPoints.reserve(samplesToDraw);

        float midY = y + height * 0.5f;
        float amplitudeScale = (height * 0.5f);
        float margin = 2.0f;

        float pixelPerSample = (float)width / (float)(samplesToDraw - 1);

        bool isFirst = true;

        for (int i = 0; i < samplesToDraw; ++i)
        {
            float sL = workBufferL[triggerOffset + i];
            float sR = workBufferR[triggerOffset + i];

            sL = juce::jlimit(-1.0f, 1.0f, sL);
            sR = juce::jlimit(-1.0f, 1.0f, sR);

            float currentMax = std::max(sL, sR);
            float currentMin = std::min(sL, sR);
            float currentMid = (sL + sR) * 0.5f;

            float xPos = x + (i * pixelPerSample);

            float progress = (xPos - x) / (float)width;
            if (progress < 0.0f) progress = 0.0f;
            if (progress > 1.0f) progress = 1.0f;

            float pinch = 1.0f;
            if (progress < 0.1f) pinch = std::sin((progress / 0.1f) * juce::MathConstants<float>::halfPi);
            else if (progress > 0.9f) pinch = std::sin(((1.0f - progress) / 0.1f) * juce::MathConstants<float>::halfPi);

            float valTop = currentMax * pinch;
            float valBot = currentMin * pinch;
            float valMid = currentMid * pinch;

            valMid = juce::jlimit(valBot, valTop, valMid);

            float yTop = midY - (valTop * amplitudeScale);
            float yBot = midY - (valBot * amplitudeScale);
            float yMid = midY - (valMid * amplitudeScale);

            yTop = juce::jlimit((float)y + margin, (float)y + height - margin, yTop);
            yBot = juce::jlimit((float)y + margin, (float)y + height - margin, yBot);
            yMid = juce::jlimit((float)y + margin, (float)y + height - margin, yMid);

            if (isFirst) { midPath.startNewSubPath(xPos, yMid); isFirst = false; }
            else { midPath.lineTo(xPos, yMid); }

            topPoints.push_back({ xPos, yTop });
            botPoints.push_back({ xPos, yBot });
        }

        if (!topPoints.empty()) {
            sidePath.startNewSubPath(topPoints[0]);
            for (size_t i = 1; i < topPoints.size(); ++i) sidePath.lineTo(topPoints[i]);
            for (int i = (int)botPoints.size() - 1; i >= 0; --i) sidePath.lineTo(botPoints[i]);
            sidePath.closeSubPath();
        }

        g.setColour(color);
        g.fillPath(sidePath);
        g.strokePath(sidePath, juce::PathStrokeType(1.0f));

        g.setColour(color);
        g.strokePath(midPath, juce::PathStrokeType(1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

private:
    std::vector<float> circularBufferL;
    std::vector<float> circularBufferR;
    int bufferSize;
    int writePos;

    std::vector<float> workBufferL;
    std::vector<float> workBufferR;
    int workBufferSize;
};

// ==============================================================================
// 3. SCROLLING PEAK WAVEFORM (ESTILO FABFILTER PRO-L 2 - DECAY SUAVE Y LIMPIO)
// ==============================================================================
class ScrollingPeakWaveform
{
public:
    ScrollingPeakWaveform() {
        visualBufferSize = 2048;
        buffer.resize(visualBufferSize, 0.0f);
        writePos = 0;
        tempPeak = 0.0f;
    }

    void modifyZoom(float wheelAmount)
    {
        float multiplier = 1.0f;
        if (wheelAmount > 0) multiplier = 0.9f;
        else if (wheelAmount < 0) multiplier = 1.1f;

        currentSamplesPerPixel *= multiplier;
        if (currentSamplesPerPixel < 16.0f) currentSamplesPerPixel = 16.0f;
        if (currentSamplesPerPixel > 1024.0f) currentSamplesPerPixel = 1024.0f;
    }

    void pushSamples(const float* inL, const float* inR, int numSamples)
    {
        const int samplesNeeded = (int)currentSamplesPerPixel;

        for (int i = 0; i < numSamples; ++i) {
            float absL = std::abs(inL[i]);
            float absR = std::abs(inR[i]);
            float currentMax = std::max(absL, absR);

            if (currentMax > tempPeak) tempPeak = currentMax;

            accumulatedSamples++;
            if (accumulatedSamples >= samplesNeeded) {
                buffer[writePos] = tempPeak;
                writePos = (writePos + 1) % visualBufferSize;

                // Aplicamos un decay suave (pierde 15% por pixel) en vez de corte a 0 absoluto
                tempPeak *= 0.85f;
                if (tempPeak < 0.001f) tempPeak = 0.0f;

                accumulatedSamples = 0;
            }
        }
    }

    void draw(juce::Graphics& g, juce::Rectangle<int> bounds, juce::Colour baseColor = juce::Colours::cyan)
    {
        juce::Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(bounds);

        float bottomY = (float)bounds.getBottom();
        float heightScale = (float)bounds.getHeight();
        int w = bounds.getWidth();

        int readIndex = writePos - 1;
        if (readIndex < 0) readIndex = visualBufferSize - 1;

        struct Slice { float x, top; bool isSilence; };
        std::vector<Slice> slices;
        slices.reserve(w);

        for (int x = w - 1; x >= 0; --x) {
            float val = buffer[readIndex];
            // Subimos muy levemente el umbral para que cierre el dibujo apenas termina el decay
            bool isSilence = (val < 0.001f);

            float currentX = (float)(bounds.getX() + x);
            float currentY = bottomY - (val * heightScale);

            if (!isSilence && std::abs(bottomY - currentY) < 1.0f) {
                currentY = bottomY - 1.0f;
            }

            slices.push_back({ currentX, currentY, isSilence });

            readIndex--;
            if (readIndex < 0) readIndex = visualBufferSize - 1;
        }

        juce::Path fillPath;
        juce::Path strokePath;
        bool wasSilence = true;

        for (size_t i = 0; i < slices.size(); ++i) {
            const auto& slice = slices[i];

            if (!slice.isSilence) {
                if (wasSilence) {
                    fillPath.startNewSubPath(slice.x, bottomY);
                    fillPath.lineTo(slice.x, slice.top);

                    strokePath.startNewSubPath(slice.x, slice.top);
                    if (i == slices.size() - 1 || slices[i + 1].isSilence) {
                        strokePath.lineTo(slice.x + 0.5f, slice.top);
                    }
                }
                else {
                    fillPath.lineTo(slice.x, slice.top);
                    strokePath.lineTo(slice.x, slice.top);
                }
                wasSilence = false;
            }
            else {
                if (!wasSilence) {
                    fillPath.lineTo(slices[i - 1].x, bottomY);
                    fillPath.closeSubPath();
                }
                wasSilence = true;
            }
        }

        if (!wasSilence && !slices.empty()) {
            fillPath.lineTo(slices.back().x, bottomY);
            fillPath.closeSubPath();
        }

        juce::ColourGradient grad(baseColor.withAlpha(0.6f), 0, bounds.getY(),
            baseColor.withAlpha(0.1f), 0, bottomY, false);
        g.setGradientFill(grad);
        g.fillPath(fillPath);

        g.setColour(baseColor.withMultipliedBrightness(1.3f));
        g.strokePath(strokePath, juce::PathStrokeType(1.2f));
    }

private:
    std::vector<float> buffer;
    int visualBufferSize;
    int writePos;
    float currentSamplesPerPixel = 1024.0f;
    int accumulatedSamples = 0;
    float tempPeak;
};