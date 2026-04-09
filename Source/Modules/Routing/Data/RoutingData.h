#pragma once
#include "SendData.h"
#include <vector>

/**
 * Contenedor de todos los ruteos de salida (Sends) de una pista.
 * Los Sidechains viven dentro de cada plugin, pero este objeto centraliza los envíos.
 */
struct TrackRoutingData {
    std::vector<SendEntry> sends;

    // Métodos de conveniencia
    void clear() { sends.clear(); }
    void addSend(int targetId) { sends.push_back({ targetId }); }
};
