#pragma once
#include <JuceHeader.h>
#include "../../Engine/Modulation/ModulationTargets.h"

enum class ModulatorShape { Sine, Saw, Square, Triangle, RandomStep };

/**
 * GridModulator: LFO Oscilador con múltiples formas de onda.
 * Cumple con rules.md: Estabilidad real-time y smoothing de audio.
 */
class GridModulator {
public:
    static constexpr double PIXELS_PER_BEAT = 80.0;
    static constexpr double PIXELS_PER_BAR = 320.0;

    static inline GridModulator* pendingModulator = nullptr;
    static inline std::atomic<int> globalModulatorIdCounter { 0 };

    GridModulator() : modId(globalModulatorIdCounter.fetch_add(1)) {
        rate.store(4.0f); // 1 Bar por defecto
        depth.store(1.0f);
        shape.store(static_cast<int>(ModulatorShape::Sine));
        
        // Inicializar smoothing (Regla 18 de rules.md)
        smoothedDepth.reset(44100.0, 0.02); // 20ms de rampa
        smoothedDepth.setCurrentAndTargetValue(1.0f);
    }
    
    GridModulator(int id) : modId(id) {
        rate.store(4.0f);
        depth.store(1.0f);
        shape.store(static_cast<int>(ModulatorShape::Sine));
        
        smoothedDepth.reset(44100.0, 0.02);
        smoothedDepth.setCurrentAndTargetValue(1.0f);
    }

    float getValueAt(double phase) {
        float r = std::max(0.001f, rate.load(std::memory_order_relaxed));
        
        // Convertir fase de PIXELES a BEATS antes de aplicar el rate (Regla de congruencia de unidades)
        double beats = phase / PIXELS_PER_BEAT;
        double p = std::fmod(beats / (double)r, 1.0);
        if (p < 0) p += 1.0;
        
        ModulatorShape s = static_cast<ModulatorShape>(shape.load(std::memory_order_relaxed));
        
        // Actualizar smoothing de profundidad
        smoothedDepth.setTargetValue(depth.load(std::memory_order_relaxed));
        float d = smoothedDepth.getNextValue();
        
        if (s == ModulatorShape::RandomStep) {
            // Usar Beats para el escalón aleatorio para que dure el tiempo correcto
            double beats = phase / PIXELS_PER_BEAT;
            double step = std::floor(beats / (double)r);
            juce::Random rnd((int)step + modId * 1337);
            return (rnd.nextFloat() * 2.0f - 1.0f) * d;
        }

        return getRawShapeValue(p) * d;
    }

    /**
     * Versión para el UI que no avanza el Smoothing del hilo de audio.
     */
    float getVisualValueAt(double phase) {
        float r = std::max(0.001f, rate.load(std::memory_order_relaxed));
        float d = depth.load(std::memory_order_relaxed);
        
        double beats = phase / PIXELS_PER_BEAT;
        double p = std::fmod(beats / (double)r, 1.0);
        if (p < 0) p += 1.0;
        
        ModulatorShape s = static_cast<ModulatorShape>(shape.load(std::memory_order_relaxed));
        if (s == ModulatorShape::RandomStep) {
            double step = std::floor(beats / (double)r);
            juce::Random rnd((int)step + modId * 1337);
            return (rnd.nextFloat() * 2.0f - 1.0f) * d;
        }

        return getRawShapeValue(p) * d;
    }

    float getRawShapeValue(double phase) {
        ModulatorShape s = static_cast<ModulatorShape>(shape.load(std::memory_order_relaxed));
        switch (s) {
            case ModulatorShape::Sine:     return std::sin(phase * 2.0 * juce::MathConstants<double>::pi);
            case ModulatorShape::Saw:      return (float)(2.0 * (phase - 0.5));
            case ModulatorShape::Square:   return (phase < 0.5) ? 1.0f : -1.0f;
            case ModulatorShape::Triangle: return (phase < 0.5) ? (float)(4.0 * phase - 1.0) : (float)(3.0 - 4.0 * phase);
            case ModulatorShape::RandomStep: {
                // Para visualización: reconstruir el valor aleatorio basado en el Id y seed
                // Nota: phase aquí ya es 0..1 relativo al ciclo.
                // Pero el RandomStep depende del tiempo absoluto. 
                // Para el 'dot', simplemente devolvemos 0 si no tenemos el tiempo absoluto,
                // pero si queremos que el punto se mueva en el UI, necesitamos el valor actual.
                return 0.0f; // Se manejará en getValueAt que tiene el tiempo absoluto
            }
        }
        return 0.0f;
    }

    void prepareToPlay(double sampleRate) {
        smoothedDepth.reset(sampleRate, 0.02);
    }

    // --- PARÁMETROS ATÓMICOS ---
    std::atomic<float> rate;
    std::atomic<float> depth;
    std::atomic<int>   shape;
    
    // --- DSP DE GRADO COMERCIAL ---
    juce::LinearSmoothedValue<float> smoothedDepth;

    juce::Array<ModTarget> targets;
    juce::CriticalSection targetsLock;

    void addTarget(const ModTarget& t) {
        juce::ScopedLock sl(targetsLock);
        if (!targets.contains(t)) targets.add(t);
    }

    void removeTarget(const ModTarget& t) {
        juce::ScopedLock sl(targetsLock);
        targets.removeFirstMatchingValue(t);
    }

    void clearTargets() {
        juce::ScopedLock sl(targetsLock);
        targets.clear();
    }

    int modId;
};
