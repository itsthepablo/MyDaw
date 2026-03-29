#pragma once
#include <JuceHeader.h>
#include "PlaylistTool.h"
#include "../PlaylistComponent.h"

class ScissorTool : public PlaylistTool {
public:
    void mouseDown(const juce::MouseEvent& e, PlaylistComponent& p) override {
        int cIdx = p.getClipAt(e.x, e.y);
        if (cIdx == -1) return;

        float absX = p.getAbsoluteXFromMouse(e.x);
        float splitX = std::round(absX / p.snapPixels) * p.snapPixels;

        auto& clip = p.clips[cIdx];

        if (splitX <= clip.startX || splitX >= clip.startX + clip.width) return;

        float newWidthLeft = splitX - clip.startX;
        float newWidthRight = clip.width - newWidthLeft;

        std::unique_ptr<juce::ScopedLock> lock;
        if (p.audioMutex) lock = std::make_unique<juce::ScopedLock>(*p.audioMutex);

        if (clip.linkedMidi) {
            auto* oldMidi = clip.linkedMidi;
            auto* newMidi = new MidiClipData();
            newMidi->name = oldMidi->name + " (Corte)";
            newMidi->startX = splitX;
            newMidi->width = newWidthRight;
            newMidi->color = oldMidi->color;

            for (auto it = oldMidi->notes.begin(); it != oldMidi->notes.end(); ) {
                if (it->x >= splitX) {
                    newMidi->notes.push_back(*it);
                    it = oldMidi->notes.erase(it);
                }
                else ++it;
            }

            oldMidi->width = newWidthLeft;
            clip.width = newWidthLeft;

            clip.trackPtr->midiClips.add(newMidi);
            p.clips.push_back({ clip.trackPtr, newMidi->startX, newMidi->width, newMidi->name, nullptr, newMidi });
        }
        else if (clip.linkedAudio) {
            auto* oldAudio = clip.linkedAudio;
            
            auto* newAudio = new AudioClipData();
            newAudio->name = oldAudio->name + " (Cortado)";
            newAudio->startX = splitX;
            newAudio->width = newWidthRight;
            newAudio->offsetX = oldAudio->offsetX + newWidthLeft;
            newAudio->isMuted = oldAudio->isMuted;
            newAudio->sourceFilePath = oldAudio->sourceFilePath;
            newAudio->fileBuffer.makeCopyOf(oldAudio->fileBuffer);
            newAudio->sourceSampleRate = oldAudio->sourceSampleRate;
            newAudio->cachedPeaksL = oldAudio->cachedPeaksL;
            newAudio->cachedPeaksR = oldAudio->cachedPeaksR;
            newAudio->cachedPeaksMid = oldAudio->cachedPeaksMid;
            newAudio->cachedPeaksSide = oldAudio->cachedPeaksSide;

            oldAudio->width = newWidthLeft;
            clip.width = newWidthLeft;

            clip.trackPtr->audioClips.add(newAudio);
            p.clips.push_back({ clip.trackPtr, newAudio->startX, newAudio->width, newAudio->name, newAudio, nullptr });
        }
        p.repaint();
    }

    void mouseDrag(const juce::MouseEvent& e, PlaylistComponent& p) override {}
    void mouseUp(const juce::MouseEvent& e, PlaylistComponent& p) override {}
    void mouseMove(const juce::MouseEvent& e, PlaylistComponent& p) override {
        p.setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }
};