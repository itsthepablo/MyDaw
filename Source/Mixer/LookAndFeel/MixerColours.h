#pragma once
#include <JuceHeader.h>

/**
 * MixerColours — Identificadores de color personalizados para el sistema de temas del Mixer.
 * Esto permite centralizar la estética en el LookAndFeel.
 */
namespace MixerColours {
    enum ColourIDs {
        mainBackground      = 0x2000100, // Fondo general de la consola
        channelBackground   = 0x2000101, // Fondo de cada canal individual
        masterBackground    = 0x2000102, // Fondo específico para el canal Master
        meterBackground     = 0x2000103, // Fondo del área del medidor
        trackStripeBase     = 0x2000104, // Color de la franja inferior (si no es el del track)
        accentOrange        = 0x2000105, // Naranja principal para controles
        accentAmber         = 0x2000106, // Ámbar para medidores
        slotEmpty           = 0x2000107, // Fondo de slot vacío
        pluginActive        = 0x2000108, // Fondo de slot con plugin activo
        sendActive          = 0x2000109  // Fondo de slot con envío activo
    };
}
