#pragma once
#include <JuceHeader.h>
#include "PlaylistTool.h"
#include "../PlaylistComponent.h"

class MuteTool : public PlaylistTool {
public:
    void mouseDown(const juce::MouseEvent& e, PlaylistComponent& p) override {
        int cIdx = p.getClipAt(e.x, e.y);
        if (cIdx == -1) return;

        std::unique_ptr<juce::ScopedLock> lock;
        if (p.audioMutex) lock = std::make_unique<juce::ScopedLock>(*p.audioMutex);

        auto& clip = p.clips[cIdx];
        if (clip.linkedAudio) {
            clip.linkedAudio->isMuted = !clip.linkedAudio->isMuted;
        }
        if (clip.linkedMidi) {
            clip.linkedMidi->isMuted = !clip.linkedMidi->isMuted;
        }
        
        p.repaint();
    }

    void mouseDrag(const juce::MouseEvent& e, PlaylistComponent& p) override {}
    void mouseUp(const juce::MouseEvent& e, PlaylistComponent& p) override {}
    void mouseMove(const juce::MouseEvent& e, PlaylistComponent& p) override {
        if (p.getClipAt(e.x, e.y) != -1) p.setMouseCursor(juce::MouseCursor::PointingHandCursor);
        else p.setMouseCursor(juce::MouseCursor::NormalCursor);
    }
};
