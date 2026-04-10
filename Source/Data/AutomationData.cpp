#include "AutomationData.h"
#include "../PianoRoll/AutomationMath.h"

// ==============================================================================
// AUTO LANE IMPLEMENTATION
// ==============================================================================

float AutoLane::getValueAt(float x) const {
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
