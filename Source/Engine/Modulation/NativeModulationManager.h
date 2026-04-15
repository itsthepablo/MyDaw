#pragma once
#include <JuceHeader.h>
#include "../../Data/Track.h"
#include "../../Engine/Modulation/ModulationTargets.h"

/**
 * NativeModulationManager: Gestor centralizado para la modulación universal.
 * Desacopla la lógica de despacho de los hilos de procesamiento principales.
 */
class NativeModulationManager {
public:
    NativeModulationManager() = default;

    /**
     * Aplica toda la modulación acumulada a los parámetros nativos de una pista.
     * Se llama una vez por bloque de audio en TrackProcessor.cpp.
     */
    void applyModulations(Track* track, double beatPhase);

private:
    /**
     * Verifica si un target tiene algún modulador asignado.
     */
    bool hasModulation(Track* track, ModTarget::Type targetType);

    /**
     * Obtiene el valor absoluto (mapeado al rango real) de la modulación.
     */
    float getAbsoluteModulatedValue(Track* track, ModTarget::Type targetType, double beatPhase, float min, float max, bool map = true);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeModulationManager)
};
