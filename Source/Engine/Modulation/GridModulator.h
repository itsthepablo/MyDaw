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

    GridModulator();
    GridModulator(int id);
    
    double getVisualPhase(double phasePixels) const;
    float getValueAt(double phase);
    float getVisualValueAt(double phase);
    float getRawShapeValue(double phase);

    void prepareToPlay(double /*sampleRate*/) {}

    // --- PARÁMETROS ATÓMICOS ---
    std::atomic<float> rate;
    std::atomic<float> depth;
    std::atomic<int>   shape;
    
    juce::Array<ModTarget> targets;
    juce::CriticalSection targetsLock;

    void addTarget(const ModTarget& t);
    void removeTarget(const ModTarget& t);
    void clearTargets();

    int modId;
};
