#pragma once
#include <JuceHeader.h>
#include "TransportState.h"

struct AudioClock {
    double sampleRate = 44100.0;
    int maxBlockSize = 512;
    double samplesPerPixel = 1.0;

    int currentSamplePos = 0;
    int blockEndSamplePos = 0;

    float currentPh = 0.0f;
    float nextPh = 0.0f;

    bool wasPlayingLastBlock = false;
    int lastPreviewPitch = -1;
    bool looped = false;

    void prepare(double s, int bufSize) {
        sampleRate = s;
        maxBlockSize = bufSize;
        samplesPerPixel = s / ((120.0 / 60.0) * 80.0); // Valor estricto 120bpm para inicializar (1 beat = 80px)
        currentSamplePos = 0;
        currentPh = 0.0f;
    }

    // Se alimenta el estado localizado desde el TransportState lock-free
    void update(int numSamples, TransportState& ts) {
        bool isPlayingNow = ts.isPlaying.load(std::memory_order_relaxed);
        float loopEndPos = ts.loopEndPos.load(std::memory_order_relaxed);
        float currentBpm = ts.bpm.load(std::memory_order_relaxed);
        
        // El reloj asegura que los cálculos de física (samples) coincidan simétricamente con el dibujo (px)
        samplesPerPixel = sampleRate / ((currentBpm / 60.0) * 80.0);

        // Si no estamos tocando, sincronizamos visualmente con la UI (Playhead)
        if (!isPlayingNow) {
            currentPh = ts.playheadPos.load(std::memory_order_relaxed);
            nextPh = currentPh;
            currentSamplePos = (long long)(currentPh * samplesPerPixel);
            blockEndSamplePos = currentSamplePos + numSamples;
            ts.currentAudioPlayhead.store(currentPh, std::memory_order_relaxed);
            return;
        }

        // Avanzar el reloj para este bloque
        currentPh = nextPh;

        // Si estamos tocando, el Clock de Audio domina el avance de la UI
        looped = false;
        double pixelsPerSample = (currentBpm / 60.0) * (80.0 / sampleRate);
        float deltaPh = (float)(numSamples * pixelsPerSample);

        nextPh = currentPh + deltaPh;

        if (nextPh >= loopEndPos) {
            nextPh = nextPh - loopEndPos;
            looped = true;
        }

        // Lógica estricta de posición de muestra basada puramente en el mapeo espacial del proyecto
        // Al calcular currentSamplePos mediante una simple multiplicación, nunca hay "drift" ni desface
        currentSamplePos = (long long)(currentPh * samplesPerPixel);
        blockEndSamplePos = currentSamplePos + numSamples;

        // Actualizamos la variable atómica para que la UI la lea
        ts.currentAudioPlayhead.store(currentPh, std::memory_order_relaxed);
    }
};
