#pragma once
#include <JuceHeader.h>
#include <cmath>

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

    // NUEVO: Estado interno para evitar la dependencia destructiva del lag de la UI
    bool isPlayingInternal = false;

    void prepare(double newSampleRate, int newBlockSize) {
        sampleRate = newSampleRate;
        blockSize = newBlockSize;
    }

    void update(int numSamples, bool isPlayingNow, float playheadPosUI, float loopEnd, double bpm) {
        double pixelsPerSec = (bpm / 60.0) * 80.0;

        // Protección matemática por si el BPM es 0
        samplesPerPixel = (pixelsPerSec > 0) ? (sampleRate / pixelsPerSec) : 1.0;

        if (isPlayingNow) {
            if (!isPlayingInternal) {
                // Justo al dar Play: El motor asume la posición visual actual de la UI
                currentPh = playheadPosUI;
                isPlayingInternal = true;
            }
            else {
                // Ya en Play: EL MOTOR MANDA. Avanza autónomamente con precisión de sample.
                currentPh = nextPh;

                // Excepción: Si el usuario hizo clic en la regla de tiempo y la UI reporta un salto brusco
                if (std::abs(playheadPosUI - currentPh) > 5.0f) {
                    currentPh = playheadPosUI;
                }
            }
        }
        else {
            // En Stop: La UI manda para que puedas arrastrar el cabezal visualmente sin problema
            currentPh = playheadPosUI;
            isPlayingInternal = false;
        }

        nextPh = currentPh;
        looped = false;

        currentSamplePos = (long long)(currentPh * samplesPerPixel);
        blockEndSamplePos = currentSamplePos + numSamples;

        if (isPlayingNow) {
            nextPh = currentPh + (float)((numSamples / sampleRate) * pixelsPerSec);

            // CORRECCIÓN DEL BUCLE: Establecemos un mínimo absoluto de 4 compases (1280 píxeles).
            // Esto evita que te atrape en 1 compás si arrastraste un clip muy corto.
            float safeLoopEnd = (loopEnd < 1280.0f) ? 1280.0f : loopEnd;

            looped = (nextPh >= safeLoopEnd);
            if (looped) {
                nextPh -= safeLoopEnd;
            }
        }
    }
};