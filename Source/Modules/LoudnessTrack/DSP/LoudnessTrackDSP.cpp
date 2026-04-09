#include "LoudnessTrackDSP.h"

void LoudnessTrackDSP::recordAnalysis(LoudnessTrackData& data, 
                                     SimpleLoudness& loudness,
                                     double currentSamplePos) 
{
    if (currentSamplePos < 0) return;
    data.history.addPoint(currentSamplePos, loudness.getShortTerm());
}
