#include "GridModulator.h"

GridModulator::GridModulator() : modId(globalModulatorIdCounter.fetch_add(1)) {
    rate.store(4.0f); // 1 Bar por defecto
    depth.store(1.0f);
    shape.store(static_cast<int>(ModulatorShape::Sine));
}

GridModulator::GridModulator(int id) : modId(id) {
    rate.store(4.0f);
    depth.store(1.0f);
    shape.store(static_cast<int>(ModulatorShape::Sine));
}

double GridModulator::getVisualPhase(double phasePixels) const {
    float r = std::max(0.001f, rate.load(std::memory_order_relaxed));
    double beats = phasePixels / PIXELS_PER_BEAT;
    double p = std::fmod(beats / (double)r, 1.0);
    if (p < 0) p += 1.0;
    return p;
}

float GridModulator::getValueAt(double phase) {
    float r = std::max(0.001f, rate.load(std::memory_order_relaxed));
    
    // Convertir fase de PIXELES a BEATS antes de aplicar el rate (Regla de congruencia de unidades)
    double beats = phase / PIXELS_PER_BEAT;
    double p = std::fmod(beats / (double)r, 1.0);
    if (p < 0) p += 1.0;
    
    ModulatorShape s = static_cast<ModulatorShape>(shape.load(std::memory_order_relaxed));
    float d = depth.load(std::memory_order_relaxed);
    
    if (s == ModulatorShape::RandomStep) {
        double step = std::floor(beats / (double)r);
        juce::Random rnd((int)step + modId * 1337);
        return (rnd.nextFloat() * 2.0f - 1.0f) * d;
    }

    return getRawShapeValue(p) * d;
}

float GridModulator::getVisualValueAt(double phase) {
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

float GridModulator::getRawShapeValue(double phase) {
    ModulatorShape s = static_cast<ModulatorShape>(shape.load(std::memory_order_relaxed));
    switch (s) {
        case ModulatorShape::Sine:     return (float)std::sin(phase * 2.0 * juce::MathConstants<double>::pi);
        case ModulatorShape::Saw:      return (float)(2.0 * (phase - 0.5));
        case ModulatorShape::Square:   return (phase < 0.5) ? 1.0f : -1.0f;
        case ModulatorShape::Triangle: return (phase < 0.5) ? (float)(4.0 * phase - 1.0) : (float)(3.0 - 4.0 * phase);
        case ModulatorShape::RandomStep: return 0.0f; 
    }
    return 0.0f;
}

void GridModulator::addTarget(const ModTarget& t) {
    juce::ScopedLock sl(targetsLock);
    if (!targets.contains(t)) targets.add(t);
}

void GridModulator::removeTarget(const ModTarget& t) {
    juce::ScopedLock sl(targetsLock);
    targets.removeFirstMatchingValue(t);
}

void GridModulator::clearTargets() {
    juce::ScopedLock sl(targetsLock);
    targets.clear();
}
