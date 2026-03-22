#pragma once
#include <JuceHeader.h>
#include <cmath>

class AudioClock {
public:
    double sampleRate = 44100.0;
    int blockSize = 512;
    bool wasPlayingLastBlock = false;
    int lastPreviewPitch = -1;

    float currentPh = 0.0f;
    float nextPh = 0.0f;
    bool looped = false;
    double samplesPerPixel = 0.0;
    long long currentSamplePos = 0;
    long long blockEndSamplePos = 0;

    bool isPlayingInternal = false;

    void prepare(double newSampleRate, int newBlockSize) {
        sampleRate = newSampleRate;
        blockSize = newBlockSize;
    }

    void update(int numSamples, bool isPlayingNow, float playheadPosUI, float loopEnd, double bpm) {
        double pixelsPerSec = (bpm / 60.0) * 80.0;
        samplesPerPixel = (pixelsPerSec > 0) ? (sampleRate / pixelsPerSec) : 1.0;

        if (isPlayingNow) {
            if (!isPlayingInternal) {
                currentPh = playheadPosUI;
                isPlayingInternal = true;
            }
            else {
                currentPh = nextPh;
                // CORRECCIÓN: Umbral aumentado de 5.0 a 80.0 (1 beat entero)
                // Esto evita que el lag de la Interfaz reinicie la posición de audio constantemente.
                if (std::abs(playheadPosUI - currentPh) > 80.0f) {
                    currentPh = playheadPosUI;
                }
            }
        }
        else {
            currentPh = playheadPosUI;
            isPlayingInternal = false;
        }

        nextPh = currentPh;
        looped = false;
        currentSamplePos = (long long)(currentPh * samplesPerPixel);
        blockEndSamplePos = currentSamplePos + numSamples;

        if (isPlayingNow) {
            nextPh = currentPh + (float)((numSamples / sampleRate) * pixelsPerSec);

            float safeLoopEnd = (loopEnd < 1280.0f) ? 1280.0f : loopEnd;

            looped = (nextPh >= safeLoopEnd);
            if (looped) nextPh -= safeLoopEnd;
        }
    }
};