/*
  ==============================================================================
    ExactMeter.h
    Medidor RMS de Precisión Matemática (Sliding Window).
    - RMS: Calcula la raíz cuadrática media sobre una ventana exacta de tiempo.
    - Peak: Detección de pico de muestra con caída suave.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <cmath>
#include <algorithm>

class ExactMeter
{
public:
    ExactMeter() {}

    void prepare(double sampleRate)
    {
        fs = sampleRate;
        
        // --- CONFIGURACIÓN DE VENTANA RMS ---
        // 300ms es el estándar "AES/VU" para medir volumen promedio.
        // Si lo quieres más rápido (tipo instantáneo), cambia a 50.0f.
        float windowMs = 300.0f; 

        windowSize = (int)(fs * (windowMs / 1000.0f));
        if (windowSize < 1) windowSize = 1;

        // Buffer circular para almacenar los cuadrados (energía)
        squaresBuffer.assign(windowSize, 0.0);
        writeIndex = 0;
        runningSum = 0.0;

        // Configuración Peak (Release)
        // Caída de aprox 20dB/segundo para que se sienta natural
        float peakReleaseMs = 1000.0f; 
        peakDecay = 1.0f - std::exp(-1.0f / ( (peakReleaseMs / 1000.0f) * fs ));

        reset();
    }

    void reset()
    {
        std::fill(squaresBuffer.begin(), squaresBuffer.end(), 0.0);
        runningSum = 0.0;
        writeIndex = 0;
        currentPeak = 0.0f;
    }

    void process(const float* channelData, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = channelData[i];
            float squared = sample * sample;
            float absSample = std::abs(sample);

            // --- 1. CÁLCULO RMS EXACTO (Sliding Window) ---
            // Restamos el valor antiguo que sale de la ventana
            runningSum -= squaresBuffer[writeIndex];
            
            // Sumamos el valor nuevo
            runningSum += squared;
            
            // Guardamos el nuevo valor en el buffer
            squaresBuffer[writeIndex] = squared;

            // Avanzamos índice circular
            writeIndex++;
            if (writeIndex >= windowSize) writeIndex = 0;

            // Corrección de errores de coma flotante (muy raro, pero por seguridad)
            if (runningSum < 0.0) runningSum = 0.0;

            // --- 2. CÁLCULO PEAK ---
            if (absSample > currentPeak) {
                currentPeak = absSample; 
            } else {
                currentPeak += (0.0f - currentPeak) * peakDecay; 
            }
        }
    }

    float getRMS() const {
        // RMS = Raíz de (Suma / Cantidad)
        return std::sqrt(runningSum / (double)windowSize);
    }

    float getPeak() const {
        return currentPeak;
    }

private:
    double fs = 44100.0;
    
    // Variables RMS (Sliding Window)
    std::vector<double> squaresBuffer;
    int windowSize = 0;
    int writeIndex = 0;
    double runningSum = 0.0;

    // Variables Peak
    float peakDecay = 0.0f;
    float currentPeak = 0.0f;
};