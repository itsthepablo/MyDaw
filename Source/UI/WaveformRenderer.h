#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h" 
#include <vector>
#include <cmath>
#include <algorithm>

/**
 * WaveformRenderer: Motor de renderizado "Pixel-Perfect" para formas de onda.
 * Implementa una jerarquía de Mip-Maps (LODs) para garantizar fluidez y nitidez
 * total en cualquier nivel de zoom, eliminando el aliasing y el temblor visual.
 */
class WaveformRenderer {
public:
    static void drawWaveform(juce::Graphics& g,
        const AudioClipData& clipData,
        juce::Rectangle<int> area,
        juce::Colour baseColor,
        WaveformViewMode viewMode,
        double hZoom,
        double clipStartSamples,
        double viewStartSamples)
    {
        const int width = area.getWidth();
        const int height = area.getHeight();

        if (!clipData.isLoaded.load(std::memory_order_relaxed)) {
            g.setColour(baseColor.brighter(0.5f));
            g.setFont(14.0f);
            g.drawText("Cargando...", area, juce::Justification::centred, true);
            return;
        }

        if (width <= 0 || height <= 0) return;

        bool isStereo = clipData.fileBuffer.getNumChannels() > 1;
        const juce::Colour fillColor = baseColor.brighter(0.1f).withAlpha(1.0f);
        
        // --- Cálculo de Escala de Alta Precisión ---
        // samplesPerPixel = (Muestras Totales) / (Ancho Total del clip en píxeles con el zoom actual)
        float baseW = clipData.originalWidth <= 0 ? clipData.width : clipData.originalWidth;
        const double samplesPerPixel = (double)clipData.fileBuffer.getNumSamples() / (double)(baseW * hZoom);
        
        // El offset de muestra para el píxel x=0 del área de dibujo
        // Se basa en la diferencia entre el inicio del clip y el inicio de la vista
        const double sampleOffset = (viewStartSamples - clipStartSamples) + (clipData.offsetX * hZoom * samplesPerPixel);

        // --- VISIBILITY CULLING ---
        juce::Rectangle<int> clipBounds = g.getClipBounds();
        int drawStartX = std::max(0, clipBounds.getX() - area.getX());
        int drawEndX = std::min(width, clipBounds.getRight() - area.getX());
        if (drawStartX >= drawEndX) return;

        // --- MODO MICRO (Zoom extremo: nivel de muestra) ---
        if (samplesPerPixel <= 1.0 && viewMode != WaveformViewMode::Spectrogram) {
            drawMicroMode(g, clipData, area, drawStartX, drawEndX, sampleOffset, samplesPerPixel, fillColor, viewMode, isStereo);
            return;
        }

        // --- MODO ESPECTROGRAMA ---
        if (viewMode == WaveformViewMode::Spectrogram) {
            if (!clipData.spectrogramImage.isNull()) {
                 const double totalW = (double)baseW * hZoom;
                 float imgPPP = totalW > 0 ? ((float)clipData.spectrogramImage.getWidth() / (float)totalW) : 1.0f;
                 int srcX = (int)((clipData.offsetX * hZoom + (double)drawStartX) * (double)imgPPP);
                 int srcW = (int)((double)(drawEndX - drawStartX) * (double)imgPPP);
                 if (srcW > 0 && srcX >= 0)
                     g.drawImage(clipData.spectrogramImage, area.getX() + drawStartX, area.getY(), drawEndX - drawStartX, area.getHeight(), srcX, 0, srcW, clipData.spectrogramImage.getHeight());
            }
            return;
        }

        // --- MODO MACRO (Pixel-Perfect con LODs y Blending) ---
        g.setColour(fillColor);
        const double midY = (double)area.getY() + (double)height / 2.0;
        const double halfH = (double)height / 2.0;

        for (int x = drawStartX; x < drawEndX; ++x) {
            const double s0 = sampleOffset + (double)x * samplesPerPixel;
            const double s1 = s0 + (double)samplesPerPixel;
            
            AudioPeak peak;

            // --- LÓGICA DE BLENDING (54 a 126 spp) ---
            if (samplesPerPixel >= 54.0 && samplesPerPixel <= 126.0) {
                float factor = (float)((samplesPerPixel - 54.0) / (126.0 - 54.0));
                
                // Pico desde muestras crudas (Modo Micro)
                AudioPeak microPeak = getPeakFromSamples(clipData.fileBuffer, s0, s1, isStereo, viewMode);
                // Pico desde LOD 0 (256)
                AudioPeak lod0Peak = getPeakFromLODInRange(clipData.cachedPeaksL, clipData.cachedPeaksR, s0, s1, 256.0, isStereo, viewMode);
                
                peak.maxPos = microPeak.maxPos + factor * (lod0Peak.maxPos - microPeak.maxPos);
                peak.minNeg = microPeak.minNeg + factor * (lod0Peak.minNeg - microPeak.minNeg);
            }
            // --- LÓGICA DE LODs ( > 126 spp) ---
            else if (samplesPerPixel > 126.0) {
                if (samplesPerPixel > 4096.0) {
                    peak = getPeakFromLODInRange(clipData.lod2PeaksL, clipData.lod2PeaksR, s0, s1, 4096.0, isStereo, viewMode);
                }
                else if (samplesPerPixel > 1024.0) {
                    peak = getPeakFromLODInRange(clipData.lod1PeaksL, clipData.lod1PeaksR, s0, s1, 1024.0, isStereo, viewMode);
                }
                else {
                    peak = getPeakFromLODInRange(clipData.cachedPeaksL, clipData.cachedPeaksR, s0, s1, 256.0, isStereo, viewMode);
                }
            }
            // --- LÓGICA DE MUESTRAS PURAS ( < 54 spp) ---
            else {
                peak = getPeakFromSamples(clipData.fileBuffer, s0, s1, isStereo, viewMode);
            }

            double yTop = midY - (double)(peak.maxPos * (float)halfH * 0.95f);
            double yBottom = midY - (double)(peak.minNeg * (float)halfH * 0.95f);
            
            if (yBottom - yTop < 1.0) yBottom = yTop + 1.0;

            g.drawVerticalLine(area.getX() + x, (float)yTop, (float)yBottom);
        }
    }

    static void drawWaveformSummary(juce::Graphics& g,
        const AudioClipData& clipData,
        juce::Rectangle<int> area,
        double hZoom,
        double clipStartUnits,
        double viewStartUnits)
    {
        if (clipData.lod2PeaksL.empty()) return;

        const int width = area.getWidth();
        const int height = area.getHeight();
        if (width <= 0 || height <= 0) return;

        // --- Cálculo de Escala de Alta Precisión ---
        float baseW = clipData.originalWidth <= 0 ? clipData.width : clipData.originalWidth;
        const double samplesPerUnit = (double)clipData.fileBuffer.getNumSamples() / (double)baseW;
        const double samplesPerPixel = samplesPerUnit / hZoom;
        
        // El resumen (minimapa) suele dibujarse desde el inicio del clip (offsetX=0 habitual)
        // pero respetamos el offsetX si existe.
        const double sampleOffset = (viewStartUnits - clipStartUnits + clipData.offsetX) * samplesPerUnit;

        const float midY = (float)area.getY() + (float)height / 2.0f;
        const float halfH = (float)height * 0.45f; 

        g.setColour(juce::Colours::white.withAlpha(0.8f)); 

        for (int x = 0; x < width; ++x) {
            double s0 = sampleOffset + (double)x * samplesPerPixel;
            double s1 = s0 + samplesPerPixel;
            AudioPeak peak = getPeakFromLODInRange(clipData.lod2PeaksL, clipData.lod2PeaksR, s0, s1, 4096.0, true, WaveformViewMode::Combined);

            g.drawVerticalLine(area.getX() + x, midY - (peak.maxPos * halfH), midY - (peak.minNeg * halfH));
        }
    }

private:
    static AudioPeak getPeakFromLODInRange(const std::vector<AudioPeak>& lodL,
        const std::vector<AudioPeak>& lodR,
        double s0, double s1,
        double samplesPerPoint,
        bool isStereo,
        WaveformViewMode viewMode)
    {
        int idx0 = (int)std::floor(s0 / samplesPerPoint);
        int idx1 = (int)std::ceil(s1 / samplesPerPoint);
        
        AudioPeak result = { 0.0f, 0.0f };
        int size = (int)lodL.size();

        if (idx0 >= size) return result;
        idx0 = std::max(0, idx0);
        idx1 = std::min(size, idx1);

        for (int i = idx0; i < idx1; ++i) {
            const auto& pL = lodL[i];
            result.maxPos = std::max(result.maxPos, pL.maxPos);
            result.minNeg = std::min(result.minNeg, pL.minNeg);

            if (isStereo && viewMode == WaveformViewMode::Combined) {
                const auto& pR = lodR[i];
                result.maxPos = std::max(result.maxPos, pR.maxPos);
                result.minNeg = std::min(result.minNeg, pR.minNeg);
            }
        }
        return result;
    }

    static AudioPeak getPeakFromSamples(const juce::AudioBuffer<float>& buffer,
        double s0, double s1,
        bool isStereo,
        WaveformViewMode viewMode)
    {
        AudioPeak result = { 0.0f, 0.0f };
        int numSamples = buffer.getNumSamples();
        int start = (int)std::floor(s0);
        int end = (int)std::ceil(s1);

        if (start >= numSamples) return result;
        start = std::max(0, start);
        end = std::min(numSamples, end);

        const float* lData = buffer.getReadPointer(0);
        const float* rData = isStereo ? buffer.getReadPointer(1) : nullptr;

        for (int s = start; s < end; ++s) {
            float l = lData[s];
            result.maxPos = std::max(result.maxPos, l);
            result.minNeg = std::min(result.minNeg, l);

            if (isStereo && viewMode == WaveformViewMode::Combined) {
                float r = rData[s];
                result.maxPos = std::max(result.maxPos, r);
                result.minNeg = std::min(result.minNeg, r);
            }
        }
        return result;
    }

    static void drawMicroMode(juce::Graphics& g, const AudioClipData& clipData, juce::Rectangle<int> area,
        int drawStartX, int drawEndX, double sampleOffset, double spp, juce::Colour color, WaveformViewMode viewMode, bool isStereo)
    {
        const float* lData = clipData.fileBuffer.getReadPointer(0);
        const float* rData = isStereo ? clipData.fileBuffer.getReadPointer(1) : nullptr;
        const int numSamples = clipData.fileBuffer.getNumSamples();
        const float midY = (float)area.getY() + (float)area.getHeight() / 2.0f;
        const float halfH = (float)area.getHeight() / 2.0f;

        g.setColour(color);
        juce::Path p;
        bool first = true;

        for (int x = drawStartX; x < drawEndX; ++x) {
            double sPos = sampleOffset + ((double)x * spp);
            int idx = (int)sPos;
            
            if (idx >= 0 && idx < numSamples) {
                // Interpolación lineal simple para suavizar el zoom extremo (FL Studio Style)
                float val = lData[idx];
                if (idx + 1 < numSamples) {
                    float nextVal = lData[idx + 1];
                    double fract = sPos - (double)idx;
                    val = (float)(val + fract * (nextVal - val));
                }

                if (isStereo && viewMode == WaveformViewMode::Combined) {
                    float rVal = rData[idx];
                    if (idx + 1 < numSamples) {
                        float nextRVal = rData[idx + 1];
                        double fract = sPos - (double)idx;
                        rVal = (float)(rVal + fract * (nextRVal - rVal));
                    }
                    val = (std::abs(val) > std::abs(rVal)) ? val : rVal;
                }

                float y = midY - (val * halfH * 0.9f);
                if (first) { p.startNewSubPath((float)area.getX() + x, y); first = false; }
                else p.lineTo((float)area.getX() + x, y);
            }
        }
        g.strokePath(p, juce::PathStrokeType(1.5f));

        // Dibujar bolitas en los puntos reales si el zoom es absurdamente alto
        if (spp < 0.1) {
            g.setColour(color.withAlpha(0.6f));
            for (int x = drawStartX; x < drawEndX; ++x) {
                double sPos = sampleOffset + ((double)x * spp);
                if (std::abs(sPos - std::round(sPos)) < (spp * 0.5)) {
                    int idx = (int)std::round(sPos);
                    if (idx >= 0 && idx < numSamples) {
                        float val = lData[idx];
                        float y = midY - (val * halfH * 0.9f);
                        g.fillEllipse((float)area.getX() + x - 2.5f, y - 2.5f, 5.0f, 5.0f);
                    }
                }
            }
        }
    }
};