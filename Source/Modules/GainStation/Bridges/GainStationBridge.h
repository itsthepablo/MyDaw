#pragma once
#include <JuceHeader.h>
#include "../Data/GainStationData.h"
#include "../../../Engine/SimpleLoudness.h"

/**
 * GainStationBridge — El "Contrato" entre la UI y el Motor de Procesamiento.
 * Permite que la interfaz controle el GainStation sin conocer la existencia del Track.
 */
class GainStationBridge {
public:
    GainStationBridge(GainStationData& data, SimpleLoudness& pre, SimpleLoudness& post)
        : dataRef(data), preLoudness(pre), postLoudness(post) {}

    // --- CONTROLES DE GANANCIA ---
    float getPreGain() const { return dataRef.preGain.load(std::memory_order_relaxed); }
    void setPreGain(float v) { dataRef.preGain.store(v, std::memory_order_relaxed); }

    float getPostGain() const { return dataRef.postGain.load(std::memory_order_relaxed); }
    void setPostGain(float v) { dataRef.postGain.store(v, std::memory_order_relaxed); }

    // --- SINCRONIZACIÓN VISUAL (MODULACIÓN) ---
    float getVisPreGain() const { return dataRef.visSync.pre.load(std::memory_order_relaxed); }
    float getVisPostGain() const { return dataRef.visSync.post.load(std::memory_order_relaxed); }
    bool hasActiveModPre() const { return dataRef.visSync.hasPre.load(std::memory_order_relaxed); }
    bool hasActiveModPost() const { return dataRef.visSync.hasPost.load(std::memory_order_relaxed); }

    // --- ESTADOS DE PROCESAMIENTO ---
    bool isPhaseInverted() const { return dataRef.isPhaseInverted.load(std::memory_order_relaxed); }
    void setPhaseInverted(bool i) { dataRef.isPhaseInverted.store(i, std::memory_order_relaxed); }

    bool isMonoActive() const { return dataRef.isMonoActive.load(std::memory_order_relaxed); }
    void setMonoActive(bool m) { dataRef.isMonoActive.store(m, std::memory_order_relaxed); }

    // --- MEDICIÓN (Loudness) ---
    float getInputLUFS() const { return preLoudness.getShortTerm(); }
    float getOutputLUFS() const { return postLoudness.getShortTerm(); }

private:
    GainStationData& dataRef;
    SimpleLoudness& preLoudness;
    SimpleLoudness& postLoudness;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainStationBridge)
};
