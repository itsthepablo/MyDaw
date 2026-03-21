#pragma once
#include <JuceHeader.h>
#include <vector>
#include "PianoRoll/AutomationMath.h"

// 1. ESTRUCTURAS BÁSICAS
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

// AutoNode vuelve a ser un agregado puro (sin constructores manuales).
// Esto arregla el error C2665 en todos tus push_back.
struct AutoNode {
    float x;
    float value;
    float tension;
    int curveType;

    bool operator<(const AutoNode& other) const { return x < other.x; }
};

// Al estar definido completamente aquí, arregla el error E0070.
struct AutoLane {
    std::vector<AutoNode> nodes;
    float defaultValue = 0.5f;

    float getValueAt(float x) const {
        if (nodes.empty()) return defaultValue;
        if (x <= nodes.front().x) return nodes.front().value;
        if (x >= nodes.back().x) return nodes.back().value;

        for (size_t i = 0; i < nodes.size() - 1; ++i) {
            if (x >= nodes[i].x && x <= nodes[i + 1].x) {
                float rangeX = nodes[i + 1].x - nodes[i].x;
                if (rangeX <= 0.0001f) return nodes[i].value;

                float t = (x - nodes[i].x) / rangeX;
                float progress = AutomationMath::getInterpolatedProgress(t, nodes[i].tension, nodes[i].curveType);
                return nodes[i].value + (nodes[i + 1].value - nodes[i].value) * progress;
            }
        }
        return defaultValue;
    }
};