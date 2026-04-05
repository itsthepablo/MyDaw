#pragma once
#include <JuceHeader.h>
#include <vector>
#include <atomic>
#include <algorithm>
#include <cmath>

/**
 * Representa un par de picos (máximo positivo y mínimo negativo) 
 * para un bloque de audio.
 */
struct AudioPeak {
    float maxPos = 0.0f;
    float minNeg = 0.0f;
};

/**
 * AudioClipData: Contenedor de datos para un clip de audio en la Playlist.
 * Gestiona el buffer PCM y una jerarquía de picos (MIP-MAPs) para el renderizado.
 */
struct AudioClipData {
    juce::String name;
    float startX = 0.0f;
    float width = 0.0f;
    float offsetX = 0.0f;
    float originalWidth = 0.0f;
    bool isMuted = false;
    bool isSelected = false;
    std::atomic<bool> isLoaded{ true };
    juce::String sourceFilePath;
    juce::AudioBuffer<float> fileBuffer;
    double sourceSampleRate = 44100.0;

    // --- Jerarquía de Mip-Maps (Caché en RAM estilo .ovw) ---
    // Nivel Base (LOD 0): 256 muestras por punto
    std::vector<AudioPeak> cachedPeaksL, cachedPeaksR, cachedPeaksMid, cachedPeaksSide;

    // Nivel Intermedio (LOD 1): 1,024 muestras por punto (Factor 4 de Base)
    std::vector<AudioPeak> lod1PeaksL, lod1PeaksR, lod1PeaksMid, lod1PeaksSide;

    // Nivel General (LOD 2): 4,096 muestras por punto (Factor 4 de LOD 1)
    std::vector<AudioPeak> lod2PeaksL, lod2PeaksR, lod2PeaksMid, lod2PeaksSide;

    // Nivel Macro (LOD 3): 16,384 muestras por punto (Factor 4 de LOD 2)
    std::vector<AudioPeak> lod3PeaksL, lod3PeaksR, lod3PeaksMid, lod3PeaksSide;

    juce::Image spectrogramImage;
    std::unique_ptr<juce::AudioThumbnail> thumbnail;

    // Constructor de copia manual para evitar errores con std::atomic y std::unique_ptr
    AudioClipData() = default;
    AudioClipData(const AudioClipData& other)
    {
        name = other.name;
        startX = other.startX;
        width = other.width;
        offsetX = other.offsetX;
        originalWidth = other.originalWidth;
        isMuted = other.isMuted;
        isSelected = other.isSelected;
        isLoaded.store(other.isLoaded.load());
        sourceFilePath = other.sourceFilePath;
        fileBuffer.makeCopyOf(other.fileBuffer);
        sourceSampleRate = other.sourceSampleRate;
        cachedPeaksL = other.cachedPeaksL;
        cachedPeaksR = other.cachedPeaksR;
        cachedPeaksMid = other.cachedPeaksMid;
        cachedPeaksSide = other.cachedPeaksSide;
        lod1PeaksL = other.lod1PeaksL;
        lod1PeaksR = other.lod1PeaksR;
        lod1PeaksMid = other.lod1PeaksMid;
        lod1PeaksSide = other.lod1PeaksSide;
        lod2PeaksL = other.lod2PeaksL;
        lod2PeaksR = other.lod2PeaksR;
        lod2PeaksMid = other.lod2PeaksMid;
        lod2PeaksSide = other.lod2PeaksSide;
        lod3PeaksL = other.lod3PeaksL;
        lod3PeaksR = other.lod3PeaksR;
        lod3PeaksMid = other.lod3PeaksMid;
        lod3PeaksSide = other.lod3PeaksSide;
        spectrogramImage = other.spectrogramImage;
        // thumbnail no se copia, debe re-crearse si es necesario.
    }

    /**
     * Genera la jerarquía de picos (Mip-Maps) siguiendo el estándar de FL Studio.
     * LOD 0 (Base): 256 samples
     * LOD 1 (Interm): 1,024 samples
     * LOD 2 (Gen): 4,096 samples
     * LOD 3 (Macro): 16,384 samples
     */
    void generateCache() {
        const int baseSamplesPerPoint = 256;
        const int numSamples = fileBuffer.getNumSamples();
        const int numChannels = fileBuffer.getNumChannels();
        if (numSamples <= 0 || numChannels <= 0) return;

        const int numBasePoints = (int)((numSamples + baseSamplesPerPoint - 1) / baseSamplesPerPoint);
        bool isStereo = numChannels > 1;

        // Reservar memoria para el Nivel Base
        cachedPeaksL.assign(numBasePoints, { 0.0f, 0.0f });
        if (isStereo) {
            cachedPeaksR.assign(numBasePoints, { 0.0f, 0.0f });
            cachedPeaksMid.assign(numBasePoints, { 0.0f, 0.0f });
            cachedPeaksSide.assign(numBasePoints, { 0.0f, 0.0f });
        }

        // --- FASE 1: Generar Nivel Base (LOD 0) ---
        const float* lData = fileBuffer.getReadPointer(0);
        const float* rData = isStereo ? fileBuffer.getReadPointer(1) : nullptr;

        for (int i = 0; i < numBasePoints; ++i) {
            int startSample = i * baseSamplesPerPoint;
            int endSample = std::min(startSample + baseSamplesPerPoint, numSamples);

            AudioPeak pL, pR, pMid, pSide;
            for (int s = startSample; s < endSample; ++s) {
                float l = lData[s];
                pL.maxPos = std::max(pL.maxPos, l);
                pL.minNeg = std::min(pL.minNeg, l);

                if (isStereo) {
                    float r = rData[s];
                    pR.maxPos = std::max(pR.maxPos, r);
                    pR.minNeg = std::min(pR.minNeg, r);

                    float mid = (l + r) * 0.5f;
                    pMid.maxPos = std::max(pMid.maxPos, mid);
                    pMid.minNeg = std::min(pMid.minNeg, mid);

                    float side = (l - r) * 0.5f;
                    pSide.maxPos = std::max(pSide.maxPos, side);
                    pSide.minNeg = std::min(pSide.minNeg, side);
                }
            }
            cachedPeaksL[i] = pL;
            if (isStereo) {
                cachedPeaksR[i] = pR;
                cachedPeaksMid[i] = pMid;
                cachedPeaksSide[i] = pSide;
            }
        }

        // --- FASE 2: Generar Niveles Macro (LOD 1, 2 y 3) ---
        auto buildLOD = [](const std::vector<AudioPeak>& source, int factor) {
            if (source.empty()) return std::vector<AudioPeak>();
            int numPoints = (int)(source.size() + factor - 1) / factor;
            std::vector<AudioPeak> lod(numPoints, { 0.0f, 0.0f });

            for (int i = 0; i < numPoints; ++i) {
                int start = i * factor;
                int end = std::min(start + factor, (int)source.size());
                AudioPeak combined = source[start];
                for (int j = start + 1; j < end; ++j) {
                    combined.maxPos = std::max(combined.maxPos, source[j].maxPos);
                    combined.minNeg = std::min(combined.minNeg, source[j].minNeg);
                }
                lod[i] = combined;
            }
            return lod;
            };

        // Generar LOD 1 (Factor 4 de Base = 1,024 muestras)
        lod1PeaksL = buildLOD(cachedPeaksL, 4);
        if (isStereo) {
            lod1PeaksR = buildLOD(cachedPeaksR, 4);
            lod1PeaksMid = buildLOD(cachedPeaksMid, 4);
            lod1PeaksSide = buildLOD(cachedPeaksSide, 4);
        }

        // Generar LOD 2 (Factor 4 de LOD 1 = 4,096 muestras)
        lod2PeaksL = buildLOD(lod1PeaksL, 4);
        if (isStereo) {
            lod2PeaksR = buildLOD(lod1PeaksR, 4);
            lod2PeaksMid = buildLOD(lod1PeaksMid, 4);
            lod2PeaksSide = buildLOD(lod1PeaksSide, 4);
        }

        // Generar LOD 3 (Factor 4 de LOD 2 = 16,384 muestras)
        lod3PeaksL = buildLOD(lod2PeaksL, 4);
        if (isStereo) {
            lod3PeaksR = buildLOD(lod2PeaksR, 4);
            lod3PeaksMid = buildLOD(lod2PeaksMid, 4);
            lod3PeaksSide = buildLOD(lod2PeaksSide, 4);
        }

        // --- FASE 3: Espectrograma (Opcional, se mantiene por compatibilidad) ---
        generateSpectrogram();
    }

private:
    void generateSpectrogram() {
        const int numSamples = fileBuffer.getNumSamples();
        if (numSamples <= 0) return;

        auto getHeatColor = [](float magnitude) -> juce::Colour {
            magnitude = juce::jlimit(0.0f, 1.0f, magnitude);
            juce::Colour c = juce::Colour(20, 22, 25);
            if (magnitude < 0.2f) return c.interpolatedWith(juce::Colours::darkred, magnitude * 5.0f);
            if (magnitude < 0.6f) return juce::Colours::darkred.interpolatedWith(juce::Colours::orange, (magnitude - 0.2f) / 0.4f);
            return juce::Colours::orange.interpolatedWith(juce::Colours::yellow, (magnitude - 0.6f) / 0.4f);
            };

        const int fftOrder = 10;
        const int fftSize = 1 << fftOrder;
        const int hopSize = 256;

        juce::dsp::FFT fft(fftOrder);
        juce::dsp::WindowingFunction<float> window((size_t)fftSize, juce::dsp::WindowingFunction<float>::hann);

        int totalFrames = numSamples / hopSize;
        if (totalFrames > 0) {
            int imgHeight = fftSize / 2;
            spectrogramImage = juce::Image(juce::Image::ARGB, totalFrames, imgHeight, true);
            juce::Image::BitmapData bmpData(spectrogramImage, juce::Image::BitmapData::writeOnly);

            std::vector<float> fftData((size_t)(fftSize * 2), 0.0f);
            const float* audioData = fileBuffer.getReadPointer(0);

            for (int frame = 0; frame < totalFrames; ++frame) {
                int startSample = frame * hopSize;
                std::fill(fftData.begin(), fftData.end(), 0.0f);
                for (int i = 0; i < fftSize; ++i) {
                    if (startSample + i < numSamples) fftData[i] = audioData[startSample + i];
                }

                window.multiplyWithWindowingTable(fftData.data(), (size_t)fftSize);
                fft.performFrequencyOnlyForwardTransform(fftData.data());

                for (int bin = 0; bin < imgHeight; ++bin) {
                    float rawMag = fftData[bin];
                    float intensity = 0.0f;
                    if (rawMag > 0.0001f) {
                        intensity = juce::jmin(1.0f, (float)std::log10(1.0f + rawMag * 50.0f) / 3.0f);
                    }
                    juce::Colour heat = getHeatColor(intensity);
                    bmpData.setPixelColour(frame, imgHeight - bin - 1, heat);
                }
            }
        }
    }
};
