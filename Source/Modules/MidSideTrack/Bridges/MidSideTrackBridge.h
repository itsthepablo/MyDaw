#pragma once
#include <JuceHeader.h>
#include "../Data/MidSideTrackData.h"

/**
 * MidSideTrackBridge — Embajador del historial Mid/Side.
 */
class MidSideTrackBridge {
public:
    MidSideTrackBridge(MidSideTrackData& data) : dataRef(data) {}

    const std::map<double, MidSideTrackData::MidSidePoint>& getPoints() const { return dataRef.history.points; }
    
    float getReferenceScale() const { return dataRef.history.referenceScaleDB; }
    int getDisplayMode() const { return dataRef.history.displayMode; }

private:
    MidSideTrackData& dataRef;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidSideTrackBridge)
};
