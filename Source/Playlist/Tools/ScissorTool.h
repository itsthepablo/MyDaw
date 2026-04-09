#pragma once
#include <JuceHeader.h>
#include "PlaylistTool.h"
#include "../PlaylistComponent.h"
#include "../../Clips/Audio/AudioClip.h"
#include "../../Clips/Midi/MidiPattern.h"

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
            MidiPattern* newMidi = new MidiPattern(*oldMidi);
            
            newMidi->setName(oldMidi->getName() + " (Corte)");
            newMidi->setStartX(splitX);
            newMidi->setWidth(newWidthRight);

            // Filtrar notas para el nuevo clip
            auto& oldNotes = oldMidi->getNotes();
            auto& newNotes = newMidi->getNotes();
            newNotes.clear();

            for (auto it = oldNotes.begin(); it != oldNotes.end(); ) {
                if (it->x >= splitX) {
                    newNotes.push_back(*it);
                    it = oldNotes.erase(it);
                }
                else ++it;
            }

            oldMidi->setWidth(newWidthLeft);
            clip.width = newWidthLeft;

            clip.trackPtr->getMidiClips().add(newMidi);
            p.clips.push_back({ clip.trackPtr, newMidi->getStartX(), newMidi->getWidth(), newMidi->getName(), nullptr, newMidi });
        }
        else if (clip.linkedAudio) {
            auto* oldAudio = clip.linkedAudio;
            AudioClip* newAudio = new AudioClip(*oldAudio);
            
            newAudio->setName(oldAudio->getName() + " (Cortado)");
            newAudio->setStartX(splitX);
            newAudio->setWidth(newWidthRight);
            newAudio->setOffsetX(oldAudio->getOffsetX() + newWidthLeft);

            oldAudio->setWidth(newWidthLeft);
            clip.width = newWidthLeft;

            clip.trackPtr->getAudioClips().add(newAudio);
            p.clips.push_back({ clip.trackPtr, newAudio->getStartX(), newAudio->getWidth(), newAudio->getName(), newAudio, nullptr });
        }
        p.repaint();
    }

    void mouseDrag(const juce::MouseEvent& e, PlaylistComponent& p) override {}
    void mouseUp(const juce::MouseEvent& e, PlaylistComponent& p) override {}
    void mouseMove(const juce::MouseEvent& e, PlaylistComponent& p) override {
        p.setMouseCursor(juce::MouseCursor::CrosshairCursor);
    }
};