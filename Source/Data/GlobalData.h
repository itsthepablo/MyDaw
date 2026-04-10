#pragma once
#include <JuceHeader.h>

// ==============================================================================
// 1. ESTRUCTURAS BÁSICAS (Plane Old Data)
// ============================================================

struct Note {
    int pitch;
    int x;
    int width;
    double frequency;

    bool operator== (const Note& other) const {
        return pitch == other.pitch && x == other.x && width == other.width;
    }
};

struct NoteDragState {
    int index;
    int startX;
    int startPitch;
    int startWidth;
};
