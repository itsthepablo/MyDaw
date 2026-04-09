#pragma once
#include <JuceHeader.h>
#include "../Data/LoudnessTrackData.h"

/**
 * LoudnessTrackBridge — Provee acceso seguro al historial de LUFS.
 */
class LoudnessTrackBridge {
public:
    LoudnessTrackBridge(LoudnessTrackData& data) : dataRef(data) {}

    const std::map<double, float>& getLoudnessPoints() const { return dataRef.history.points; }
    
    float getReferenceLUFS() const { return dataRef.history.referenceLUFS; }
    void setReferenceLUFS(float v) { dataRef.history.referenceLUFS = v; }

    bool isActive() const { return dataRef.history.isActive; }
    void setActive(bool active) { dataRef.history.isActive = active; }

private:
    LoudnessTrackData& dataRef;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoudnessTrackBridge)
};
