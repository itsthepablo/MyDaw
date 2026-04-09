#pragma once
#include <JuceHeader.h>
#include "../Data/GainStationData.h"

// Forward declarations para evitar dependencias circulares pesadas
class Track;

/**
 * GainStationDSP — Maneja el procesamiento de audio del módulo GainStation.
 * Realiza Ganancia de Entrada/Salida, Inversión de Fase y Suma a Mono.
 */
class GainStationDSP {
public:
    static void processPreFX(GainStationData& data, Track* track, juce::AudioBuffer<float>& buffer);
    static void processPostFX(GainStationData& data, Track* track, juce::AudioBuffer<float>& buffer);
};
