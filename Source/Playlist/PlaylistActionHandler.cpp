#include "PlaylistActionHandler.h"
#include "PlaylistComponent.h"
#include <cmath>
#include <algorithm>

void PlaylistActionHandler::deleteClip(PlaylistComponent& p, int index) {
    if (index < 0 || index >= (int)p.clips.size()) return;

    auto& tc = p.clips[index];

    if (tc.linkedMidi && p.onMidiClipDeleted) p.onMidiClipDeleted(tc.linkedMidi);

    std::unique_ptr<juce::ScopedLock> lock;
    if (p.audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*p.audioMutex);

    if (p.trackContainer) {
        if (tc.linkedAudio) {
            tc.trackPtr->audioClips.removeObject(tc.linkedAudio, false);
            p.trackContainer->unusedAudioPool.add(tc.linkedAudio);
        }
        if (tc.linkedMidi) {
            tc.trackPtr->midiClips.removeObject(tc.linkedMidi, false);
            p.trackContainer->unusedMidiPool.add(tc.linkedMidi);
        }
    }
    else {
        if (tc.linkedAudio) tc.trackPtr->audioClips.removeObject(tc.linkedAudio, true);
        if (tc.linkedMidi) tc.trackPtr->midiClips.removeObject(tc.linkedMidi, true);
    }

    p.clips.erase(p.clips.begin() + index);

    if (p.selectedClipIndex == index) p.selectedClipIndex = -1;
    else if (p.selectedClipIndex > index) p.selectedClipIndex--;

    p.repaint();
}

void PlaylistActionHandler::deleteClipsByName(PlaylistComponent& p, const juce::String& name, bool isMidi) {
    std::unique_ptr<juce::ScopedLock> lock;
    if (p.audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*p.audioMutex);

    for (int i = (int)p.clips.size() - 1; i >= 0; --i) {
        auto& c = p.clips[i];
        if (isMidi && c.linkedMidi && c.linkedMidi->name == name) {
            if (p.onMidiClipDeleted) p.onMidiClipDeleted(c.linkedMidi);
            c.trackPtr->midiClips.removeObject(c.linkedMidi, true);
            p.clips.erase(p.clips.begin() + i);
        }
        else if (!isMidi && c.linkedAudio && c.linkedAudio->name == name) {
            c.trackPtr->audioClips.removeObject(c.linkedAudio, true);
            p.clips.erase(p.clips.begin() + i);
        }
    }
    p.selectedClipIndex = -1;
    p.updateScrollBars();
    p.repaint();
}

void PlaylistActionHandler::purgeClipsOfTrack(PlaylistComponent& p, Track* track) {
    p.clips.erase(std::remove_if(p.clips.begin(), p.clips.end(),
        [track](const TrackClip& c) { return c.trackPtr == track; }),
        p.clips.end());
}

void PlaylistActionHandler::handleDoubleClick(PlaylistComponent& p, const juce::MouseEvent& e) {
    if (!p.tracksRef) return;
    int cIdx = p.getClipAt(e.x, e.y);

    if (cIdx != -1 && p.clips[cIdx].linkedMidi != nullptr) {
        if (p.onMidiClipDoubleClicked) {
            p.onMidiClipDoubleClicked(p.clips[cIdx].trackPtr, p.clips[cIdx].linkedMidi);
        }
        return;
    }

    int tIdx = p.getTrackAtY(e.y);
    if (tIdx != -1 && (*p.tracksRef)[tIdx]->getType() == TrackType::MIDI && cIdx == -1) {
        float absX = (e.x + (float)p.hNavigator.getCurrentRangeStart()) / p.hZoom;
        float snappedX = std::round(absX / p.snapPixels) * p.snapPixels;

        Track* targetTrack = (*p.tracksRef)[tIdx];
        MidiClipData* newMidiClip = nullptr;

        if (!targetTrack->midiClips.isEmpty()) {
            MidiClipData* sourceClip = targetTrack->midiClips.getLast();
            newMidiClip = new MidiClipData(*sourceClip);

            float timeShift = snappedX - sourceClip->startX;
            newMidiClip->startX = snappedX;

            for (auto& note : newMidiClip->notes) {
                note.x += timeShift;
            }

            auto shiftAutoLane = [timeShift](AutoLane& lane) {
                for (auto& node : lane.nodes) {
                    node.x += timeShift;
                }
            };
            shiftAutoLane(newMidiClip->autoVol);
            shiftAutoLane(newMidiClip->autoPan);
            shiftAutoLane(newMidiClip->autoPitch);
            shiftAutoLane(newMidiClip->autoFilter);
        }
        else {
            int maxPatternNum = 0;
            for (auto* tr : *p.tracksRef) {
                for (auto* mc : tr->midiClips) {
                    if (mc->name.startsWith("Pattern ")) {
                        int num = mc->name.substring(8).getIntValue();
                        if (num > maxPatternNum) maxPatternNum = num;
                    }
                }
            }
            int nextPatternNum = maxPatternNum + 1;

            newMidiClip = new MidiClipData();
            newMidiClip->name = "Pattern " + juce::String(nextPatternNum);
            newMidiClip->startX = snappedX;
            newMidiClip->width = 320.0f;
            newMidiClip->color = targetTrack->getColor();
        }

        targetTrack->midiClips.add(newMidiClip);
        p.clips.push_back({ targetTrack, snappedX, newMidiClip->width, newMidiClip->name, nullptr, newMidiClip });
        p.repaint();
    }
}