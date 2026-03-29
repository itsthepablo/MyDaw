#pragma once
#include <JuceHeader.h>
#include "PlaylistTool.h"
#include "../PlaylistComponent.h"

class EraserTool : public PlaylistTool {
public:
    void mouseDown(const juce::MouseEvent& e, PlaylistComponent& p) override {
        int cIdx = p.getClipAt(e.x, e.y);
        if (cIdx != -1) {
            if (std::find(p.selectedClipIndices.begin(), p.selectedClipIndices.end(), cIdx) == p.selectedClipIndices.end()) {
                 p.selectedClipIndices.clear();
                 p.selectedClipIndices.push_back(cIdx);
            }
            p.deleteSelectedClips();
        }
    }
    
    void mouseDrag(const juce::MouseEvent& e, PlaylistComponent& p) override {}
    void mouseUp(const juce::MouseEvent& e, PlaylistComponent& p) override {}
    
    void mouseMove(const juce::MouseEvent& e, PlaylistComponent& p) override {
        int cIdx = p.getClipAt(e.x, e.y);
        if (cIdx != -1) {
            p.setMouseCursor(juce::MouseCursor::CrosshairCursor);
        } else {
            p.setMouseCursor(juce::MouseCursor::NormalCursor);
        }
    }
};