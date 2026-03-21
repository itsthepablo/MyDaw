#pragma once
#include <JuceHeader.h>

class AudioClock {
public:
    double sampleRate = 44100.0;
    int blockSize = 512;
    bool wasPlayingLastBlock = false;
    int lastPreviewPitch = -1;

    // Resultados del cálculo temporal
    float currentPh = 0.0f;
    float nextPh = 0.0f;
    bool looped = false;
    double samplesPerPixel = 0.0;
    long long currentSamplePos = 0;
    long long blockEndSamplePos = 0;

    void prepare(double newSampleRate, int newBlockSize) {
        sampleRate = newSampleRate;
        blockSize = newBlockSize;
    }

    void update(int numSamples, bool isPlayingNow, float playheadPos, float loopEnd, double bpm) {
        currentPh = playheadPos;
        nextPh = currentPh;
        looped = false;

        double pixelsPerSec = (bpm / 60.0) * 80.0;

        // Protección matemática por si el BPM es 0
        if (pixelsPerSec > 0) {
            samplesPerPixel = sampleRate / pixelsPerSec;
        }
        else {
            samplesPerPixel = 1.0;
        }

        currentSamplePos = (long long)(currentPh * samplesPerPixel);
        blockEndSamplePos = currentSamplePos + numSamples;

        if (isPlayingNow) {
            nextPh = currentPh + (float)((numSamples / sampleRate) * pixelsPerSec);
            looped = (nextPh >= loopEnd);
            if (looped) nextPh -= loopEnd;
        }
    }
};