#pragma once
#include <JuceHeader.h>
#include <vector>
#include "../PianoRoll/AutomationMath.h"

// ============================================================
// AUTOMATION DATA MODELS
// ============================================================

struct AutoNode {
    float x;
    float value;
    float tension;
    int curveType;

    bool operator<(const AutoNode& other) const { return x < other.x; }
};

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

struct AutomationClipData {
    juce::String name;
    int targetTrackId = -1;
    int parameterId = 0; // 0 = Volume, 1 = Pan
    AutoLane lane;
    juce::Colour color;
    bool isShowing = false;
};

struct AutomationClipSnapshot {
    int parameterId = 0;
    AutoLane lane;
};
