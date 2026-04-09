#include "PlaylistAnalyzer.h"

void PlaylistAnalyzer::recordAnalysisData(const juce::OwnedArray<Track>* tracksRef, 
                                          Track* masterTrackPtr, 
                                          float playheadAbsPos, 
                                          bool isDraggingTimeline)
{
    // --- RECORD LOUDNESS HISTORY (AUTOMATIC) ---
    if (tracksRef && masterTrackPtr && !isDraggingTimeline) {
        for (auto* t : *tracksRef) {
            if (t->getType() == TrackType::Loudness) {
                float lufs = masterTrackPtr->dsp.postLoudness.getShortTerm();
                double samplePos = (double)playheadAbsPos; 
                t->loudnessTrackData.history.addPoint(samplePos, lufs);
            } else if (t->getType() == TrackType::Balance) {
                float bal = masterTrackPtr->dsp.postBalance.getBalance();
                double samplePos = (double)playheadAbsPos;
                t->balanceTrackData.history.addPoint(samplePos, bal);
            } else if (t->getType() == TrackType::MidSide) {
                float m = masterTrackPtr->dsp.postMidSide.getMid();
                float s = masterTrackPtr->dsp.postMidSide.getSide();
                float c = masterTrackPtr->dsp.postMidSide.getPhaseCorrelation();
                double samplePos = (double)playheadAbsPos;
                t->midSideTrackData.history.addPoint(samplePos, m, s, c);
            }
        }
    }
}
