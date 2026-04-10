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

    float getValueAt(float x) const;
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
