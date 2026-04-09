#pragma once
#include <JuceHeader.h>
#include <atomic>

/**
 * Configuración de una conexión de Sidechain para un efecto específico.
 * En el futuro, este objeto podrá contener configuraciones de EQ (Filtros).
 */
struct SidechainSettings {
    std::atomic<int> sourceTrackId { -1 };
    
    // --- FUTURAS MEJORAS (IDEAS.MD) ---
    // bool isEqActive = false;
    // float lowCutFreq = 20.0f;
    // float highCutFreq = 20000.0f;
};
