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
                float lufs = masterTrackPtr->postLoudness.getShortTerm();
                double samplePos = (double)playheadAbsPos; 
                t->loudnessHistory.addPoint(samplePos, lufs);
            } else if (t->getType() == TrackType::Balance) {
                float bal = masterTrackPtr->postBalance.getBalance();
                double samplePos = (double)playheadAbsPos;
                t->balanceHistory.addPoint(samplePos, bal);
            } else if (t->getType() == TrackType::MidSide) {
                float m = masterTrackPtr->postMidSide.getMid();
                float s = masterTrackPtr->postMidSide.getSide();
                float c = masterTrackPtr->postMidSide.getPhaseCorrelation();
                double samplePos = (double)playheadAbsPos;
                t->midSideHistory.addPoint(samplePos, m, s, c);
            }
        }
    }
}
