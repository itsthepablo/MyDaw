#pragma once
#include <JuceHeader.h>
#include "../Data/BalanceTrackData.h"

/**
 * BalanceTrackBridge — Embajador del historial de Balance.
 */
class BalanceTrackBridge {
public:
    BalanceTrackBridge(BalanceTrackData& data) : dataRef(data) {}

    const std::map<double, float>& getPoints() const { return dataRef.history.points; }
    float getReferenceScale() const { return dataRef.history.referenceScaleDB; }
    bool isActive() const { return dataRef.history.isActive; }

private:
    BalanceTrackData& dataRef;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BalanceTrackBridge)
};
