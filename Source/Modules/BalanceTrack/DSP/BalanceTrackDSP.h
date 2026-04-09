#pragma once
#include <JuceHeader.h>
#include "../Data/BalanceTrackData.h"
#include "../../../Engine/SimpleBalance.h"

class BalanceTrackDSP {
public:
    static void recordAnalysis(BalanceTrackData& data, 
                               SimpleBalance& balance,
                               double currentSamplePos)
    {
        if (currentSamplePos < 0) return;
        data.history.addPoint(currentSamplePos, balance.getBalance());
    }
};
