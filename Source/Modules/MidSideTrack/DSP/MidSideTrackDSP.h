#pragma once
#include <JuceHeader.h>
#include "../Data/MidSideTrackData.h"
#include "../../../Engine/SimpleMidSide.h"

class MidSideTrackDSP {
public:
    static void recordAnalysis(MidSideTrackData& data, 
                               SimpleMidSide& ms,
                               double currentSamplePos)
    {
        if (currentSamplePos < 0) return;
        data.history.addPoint(currentSamplePos, ms.getMid(), ms.getSide(), ms.getPhaseCorrelation());
    }
};
