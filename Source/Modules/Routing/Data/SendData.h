#pragma once
#include <JuceHeader.h>

/** 
 * Define si el envío toma la señal estéreo, solo el canal Mid o solo Side.
 */
enum class RoutingMode { Stereo, Mid, Side };

/**
 * Representa un punto de envío de audio de una pista a otra.
 */
struct SendEntry {
    int targetTrackId = -1;
    float gain = 1.0f;
    bool isPreFader = false;
    bool isMuted = false;
    RoutingMode mode = RoutingMode::Stereo;
};
