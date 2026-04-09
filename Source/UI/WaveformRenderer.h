#pragma once
#include <JuceHeader.h>
#include "../Data/Track.h" 
#include <vector>
#include <cmath>
#include <algorithm>

/**
 * WaveformRenderer: Motor de renderizado de Grado Comercial.
 * Implementa Mip-mapping de Datos (Interpolación Lineal entre LODs).
 * Erradica el jitter mediante Estabilización de Picos (Coordenadas Sub-píxel).
 */
class WaveformRenderer {
public:
    static void drawWaveform(juce::Graphics& g,
        const AudioClipData& clipData,
        juce::Rectangle<float> area,
        juce::Colour baseColor,
        WaveformViewMode viewMode,
        double hZoom,
        double clipStartSamples,
        double viewStartSamples)
    {
        if (!clipData.isLoaded.load(std::memory_order_relaxed)) {
            g.setColour(baseColor.brighter(0.5f));
            g.setFont(14.0f);
            g.drawText("Cargando...", area, juce::Justification::centred, true);
            return;
        }

        const int width = area.getWidth();
        const int height = area.getHeight();
        if (width <= 0 || height <= 0) return;

        // Blindaje de Seguridad (Safety Shield)
        if (std::isnan(hZoom) || hZoom <= 0.00000001) return;

        float baseW = clipData.originalWidth <= 0 ? clipData.width : clipData.originalWidth;
        const double samplesPerUnit = (double)clipData.fileBuffer.getNumSamples() / (double)baseW;
        const double samplesPerPixel = samplesPerUnit / hZoom;

        if (std::isnan(samplesPerPixel) || samplesPerPixel <= 0.0) return;

        const double pixelsPerSample = 1.0 / samplesPerPixel;
        const double visibleStartS = viewStartSamples - clipStartSamples + (clipData.offsetX * samplesPerUnit);
        const double visibleEndS = visibleStartS + (double)width * samplesPerPixel;

        // --- MODO ESPECTROGRAMA ---
        if (viewMode == WaveformViewMode::Spectrogram) {
            drawSpectrogram(g, clipData, area, hZoom, pixelsPerSample, visibleStartS, baseW);
            return;
        }

        // --- RENDERIZADO DE FORMA DE ONDA TRANSICIONAL (ZERO JITTER) ---
        g.setColour(baseColor.brighter(0.1f));
        
        if (viewMode == WaveformViewMode::SeparateLR && clipData.fileBuffer.getNumChannels() > 1) {
            float h = area.getHeight() / 2.0f;
            renderSeamlessLayer(g, clipData, area.withHeight(h), visibleStartS, visibleEndS, pixelsPerSample, samplesPerPixel, WaveformViewMode::SeparateLR, 0, baseColor); // Left
            renderSeamlessLayer(g, clipData, area.withY(area.getY() + h).withHeight(h), visibleStartS, visibleEndS, pixelsPerSample, samplesPerPixel, WaveformViewMode::SeparateLR, 1, baseColor); // Right
        } else {
            renderSeamlessLayer(g, clipData, area, visibleStartS, visibleEndS, pixelsPerSample, samplesPerPixel, viewMode, 0, baseColor);
        }
    }

private:
    static double calculateLODLevel(double spp) {
        if (spp <= 1.0) return -1.0; 
        if (spp <= 256.0) return (std::log(spp) / std::log(256.0)) - 1.0; 
        return std::log(spp / 256.0) / std::log(4.0);
    }

    /**
     * getBlendedPeak: Preservación íntegra de la lógica de transición.
     */
    static AudioPeak getBlendedPeak(const AudioClipData& clipData, double s, int level, WaveformViewMode mode, int chanIdx) {
        if (level < -1) level = -1;
        if (level > 3) level = 3;

        auto selectData = [&](int lvl) -> const std::vector<AudioPeak>* {
            if (lvl < 0) return nullptr;
            bool useMS = (mode == WaveformViewMode::MidSide);
            if (lvl == 0) return useMS ? ((chanIdx == 0) ? &clipData.cachedPeaksMid : &clipData.cachedPeaksSide) : ((chanIdx == 0) ? &clipData.cachedPeaksL : &clipData.cachedPeaksR);
            if (lvl == 1) return useMS ? ((chanIdx == 0) ? &clipData.lod1PeaksMid : &clipData.lod1PeaksSide) : ((chanIdx == 0) ? &clipData.lod1PeaksL : &clipData.lod1PeaksR);
            if (lvl == 2) return useMS ? ((chanIdx == 0) ? &clipData.lod2PeaksMid : &clipData.lod2PeaksSide) : ((chanIdx == 0) ? &clipData.lod2PeaksL : &clipData.lod2PeaksR);
            return useMS ? ((chanIdx == 0) ? &clipData.lod3PeaksMid : &clipData.lod3PeaksSide) : ((chanIdx == 0) ? &clipData.lod3PeaksL : &clipData.lod3PeaksR);
        };

        auto sampleVec = [&](const std::vector<AudioPeak>* vec, double sppLOD) -> AudioPeak {
            if (!vec || vec->empty()) return { 0.0f, 0.0f };
            double idx = s / sppLOD;
            int i0 = (int)std::floor(idx);
            int i1 = i0 + 1;
            float frac = (float)(idx - i0);
            i0 = std::max(0, std::min((int)vec->size() - 1, i0));
            i1 = std::max(0, std::min((int)vec->size() - 1, i1));
            return { (*vec)[i0].maxPos * (1.0f - frac) + (*vec)[i1].maxPos * frac, (*vec)[i0].minNeg * (1.0f - frac) + (*vec)[i1].minNeg * frac };
        };

        if (level < 0) {
            int n = clipData.fileBuffer.getNumSamples();
            int i0 = (int)std::floor(s), i1 = i0 + 1;
            float frac = (float)(s - i0);
            i0 = std::max(0, std::min(n - 1, i0)); i1 = std::max(0, std::min(n - 1, i1));
            auto* dataL = clipData.fileBuffer.getReadPointer(0);
            auto* dataR = (clipData.fileBuffer.getNumChannels() > 1) ? clipData.fileBuffer.getReadPointer(1) : dataL;
            auto getV = [&](const float* data) { return data[i0] * (1.0f - frac) + data[i1] * frac; };
            if (mode == WaveformViewMode::MidSide) {
                float v = (chanIdx == 0) ? (getV(dataL) + getV(dataR)) * 0.5f : (getV(dataL) - getV(dataR)) * 0.5f;
                return { v, v };
            }
            float val = (chanIdx == 0) ? getV(dataL) : getV(dataR);
            return { val, val };
        }
        return sampleVec(selectData(level), (level == 0) ? 256.0 : (level == 1) ? 1024.0 : (level == 2) ? 4096.0 : 16384.0);
    }

    /**
     * renderSeamlessLayer: MOTOR GEN-4 HYBRID-ELITE (Zero-Jitter + Zero-Lag).
     * Sustituye juce::Path por dibujo de precisión sub-píxel mediante fillRect(float).
     * Preserva: Zero-Jitter, Smooth Transitions y todos los comentarios originales.
     */
    static void renderSeamlessLayer(juce::Graphics& g, const AudioClipData& clipData, juce::Rectangle<float> area,
        double startSample, double endSample, double pixelsPerSample, double spp, WaveformViewMode mode, int chanIdx, juce::Colour color)
    {
        juce::Graphics::ScopedSaveState ss(g); // Blindaje de Higiene Gráfica (Protege Colores/Opacidad)
        const float midY = (float)area.getY() + (float)area.getHeight() / 2.0f;
        const float halfH = (float)area.getHeight() / 2.0f;
        const bool isStereo = clipData.fileBuffer.getNumChannels() > 1;

        // Limites de Seguridad de Coordenadas (Atomic Visibility Protection)
        const float minX = area.getX() - 10.0f;
        const float maxX = area.getRight() + 10.0f;
        const float minY = area.getY() - 10.0f;
        const float maxY = area.getBottom() + 10.0f;
        auto clampX = [&](float x) { return std::max(minX, std::min(maxX, x)); };
        auto clampY = [&](float y) { return std::max(minY, std::min(maxY, y)); };

        // --- ZONA DE ALTO RENDIMIENTO: CACHÉ DE PUNTEROS ---
        const int numBuffSamples = clipData.fileBuffer.getNumSamples();
        const float* dataL = clipData.fileBuffer.getReadPointer(0);
        const float* dataR = (isStereo && (chanIdx == 1 || mode == WaveformViewMode::Combined)) ? clipData.fileBuffer.getReadPointer(1) : nullptr;
        const float* activeData = (chanIdx == 1 && dataR) ? dataR : dataL;

        // --- MOTOR DE ZOOM INFINITO (MICRO) [PRE-FETCHED DATA] ---
        if (spp < 2.0) {
            long long s0 = (long long)std::floor(startSample);
            long long s1 = (long long)std::ceil(endSample);
            float lastX = 0.0f, lastY = 0.0f;
            bool first = true;
            
            for (long long i = s0; i <= s1; ++i) {
                if (i < 0 || i >= (long long)numBuffSamples) continue;
                
                float val = activeData[i];
                if (isStereo && mode == WaveformViewMode::Combined && dataR) {
                    float valR = dataR[i];
                    val = (std::abs(val) > std::abs(valR)) ? val : valR; // Peak Selection
                }
                
                float px = clampX((float)((double)(i - startSample) * pixelsPerSample) + (float)area.getX());
                float py = clampY(midY - (val * halfH * 0.95f));
                
                if (!first) {
                    if (px > (float)area.getX() - 5.0f && lastX < (float)area.getRight() + 5.0f) {
                        g.drawLine(lastX, lastY, px, py, 1.5f);
                    }
                }
                
                // Sample Dot (Dots dinámicos solo si hay espacio)
                if (pixelsPerSample > 6.0f) {
                    float dotSize = std::min(6.0f, (float)(pixelsPerSample * 0.3f));
                    if (dotSize > 2.0f && px >= (float)area.getX() && px <= (float)area.getRight()) {
                        g.fillEllipse(px - (dotSize/2.0f), py - (dotSize/2.0f), dotSize, dotSize);
                    }
                }
                
                lastX = px; lastY = py; first = false;
            }
            return;
        }

        // --- MOTOR MACRO ADAPTATIVO (HYBRID-ELITE) [ZERO-JITTER + ZERO-LAG] ---
        // ZONA SAGRADA: ANCLAJE DE MUESTRAS + SMOOTH TRANSITIONS
        double lodVal = calculateLODLevel(spp);
        int levelFine = (int)std::floor(lodVal);
        int levelCoarse = levelFine + 1;
        float alpha = (float)(lodVal - levelFine);

        double sppLOD = (levelFine < 0) ? 1.0 : (levelFine == 0) ? 256.0 : (levelFine == 1) ? 1024.0 : (levelFine == 2) ? 4096.0 : 16384.0;
        long long idxStart = (long long)std::floor(startSample / sppLOD) - 1;
        long long idxEnd = (long long)std::ceil(endSample / sppLOD) + 1;

        // ** SAMPLE-ANCHORED DRAWING: 1 Línea por pico real (Cero Jitter) **
        for (long long i = idxStart; i < idxEnd; ++i) {
            // El secreto de MyPianoRoll (Anclaje Sub-píxel)
            double s = (double)i * sppLOD;
            
            AudioPeak pFine = getBlendedPeak(clipData, s, levelFine, mode, chanIdx);
            AudioPeak pCoarse = getBlendedPeak(clipData, s, levelCoarse, mode, chanIdx);
            
            float maxV = pFine.maxPos * (1.0f - alpha) + pCoarse.maxPos * alpha;
            float minV = pFine.minNeg * (1.0f - alpha) + pCoarse.minNeg * alpha;
            
            if (isStereo && mode == WaveformViewMode::Combined) {
                AudioPeak pfR = getBlendedPeak(clipData, s, levelFine, mode, 1);
                AudioPeak pcR = getBlendedPeak(clipData, s, levelCoarse, mode, 1);
                maxV = std::max(maxV, pfR.maxPos * (1.0f - alpha) + pcR.maxPos * alpha);
                minV = std::min(minV, pfR.minNeg * (1.0f - alpha) + pcR.minNeg * alpha);
            }
            
            float px = (float)((s - startSample) * pixelsPerSample) + (float)area.getX();
            float pyTop = clampY(midY - (maxV * halfH * 0.95f));
            float pyBottom = clampY(midY - (minV * halfH * 0.95f));
            
            // DIBUJO DE ALTA PRECISIÓN (Zero Jitter Hardcore)
            // Usamos fillRect con float en lugar de drawVerticalLine(int) para activar el anti-aliasing de sub-píxel.
            if (px >= area.getX() && px < area.getRight()) {
                g.fillRect(juce::Rectangle<float>(px, pyTop, 1.0f, pyBottom - pyTop));
                
                // Efecto de contorno estético (Stroke simulado)
                if (spp < 64.0) {
                    g.setColour(color.withAlpha(0.3f));
                    g.fillRect(juce::Rectangle<float>(px, pyTop - 1.0f, 1.0f, 2.0f));
                    g.fillRect(juce::Rectangle<float>(px, pyBottom - 1.0f, 1.0f, 2.0f));
                    g.setColour(color.withAlpha(1.0f));
                }
            }
        }

        // Overlay MicroPuntos (Brillo adicional en ondas densas)
        if (spp < 32.0) {
            float pointAlpha = std::max(0.0f, (float)(16.0 - spp) / 16.0f);
            if (pointAlpha > 0.05f) {
                g.setColour(juce::Colours::white.withAlpha(pointAlpha * 0.25f));
                for (long long i = idxStart; i < idxEnd; i += 2) {
                    double s = (double)i * sppLOD;
                    AudioPeak p = getBlendedPeak(clipData, s, -1, mode, chanIdx);
                    float px = (float)((s - startSample) * pixelsPerSample) + (float)area.getX();
                    float py = midY - (p.maxPos * halfH * 0.95f);
                    if (px >= area.getX() && px <= area.getRight())
                        g.fillEllipse(px - 1.0f, py - 1.0f, 2.0f, 2.0f);
                }
            }
        }
    }

    static void drawSpectrogram(juce::Graphics& g, const AudioClipData& clipData, juce::Rectangle<float> area, 
        double hZoom, double pps, double startSample, float baseW) 
    {
        if (clipData.spectrogramImage.isNull()) return;
        const double totalW = (double)baseW * hZoom;
        float imgPPP = (float)clipData.spectrogramImage.getWidth() / (float)totalW;
        int srcX = (int)std::floor(startSample * (pps * imgPPP));
        int srcW = (int)std::ceil((double)area.getWidth() * (pps * imgPPP));
        if (srcW > 0 && srcX >= 0)
            g.drawImage(clipData.spectrogramImage, area.getX(), area.getY(), area.getWidth(), area.getHeight(), srcX, 0, srcW, clipData.spectrogramImage.getHeight());
    }

public:
    static void drawWaveformSummary(juce::Graphics& g, const AudioClipData& clipData, juce::Rectangle<float> area, 
        double hZoom, double clipStartUnits, double viewStartUnits) 
    {
        if (clipData.lod2PeaksL.empty() || hZoom <= 0.0) return;
        float baseW = clipData.originalWidth <= 0 ? clipData.width : clipData.originalWidth;
        const double samplesPerUnit = (double)clipData.fileBuffer.getNumSamples() / (double)baseW;
        const double samplesPerPixel = samplesPerUnit / hZoom;
        const double pixelsPerSample = 1.0 / std::max(0.0000001, samplesPerPixel);
        const double startSample = (viewStartUnits - clipStartUnits + clipData.offsetX) * samplesPerUnit;
        g.setOpacity(0.8f);
        renderSeamlessLayer(g, clipData, area, startSample, startSample + area.getWidth() * samplesPerPixel, pixelsPerSample, samplesPerPixel, WaveformViewMode::Combined, 0, juce::Colours::white);
    }
};