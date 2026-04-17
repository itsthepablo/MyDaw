#include "PlaylistActionHandler.h"
#include "PlaylistComponent.h"
#include <cmath>
#include <algorithm>

void PlaylistActionHandler::deleteSelectedClips(PlaylistComponent& p) {
    if (p.selectedClipIndices.empty()) return;

    std::unique_ptr<juce::ScopedLock> lock;
    if (p.audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*p.audioMutex);

    std::sort(p.selectedClipIndices.begin(), p.selectedClipIndices.end(), std::greater<int>());

    for (int index : p.selectedClipIndices) {
        if (index < 0 || index >= (int)p.clips.size()) continue;
        auto& tc = p.clips[index];

        if (tc.linkedMidi && p.onMidiClipDeleted) p.onMidiClipDeleted(tc.linkedMidi);

        if (p.trackContainer) {
            if (tc.linkedAudio) {
                tc.trackPtr->getAudioClips().removeObject(tc.linkedAudio, false);
                p.trackContainer->unusedAudioPool.add(tc.linkedAudio);
            }
            if (tc.linkedMidi) {
                tc.trackPtr->getMidiClips().removeObject(tc.linkedMidi, false);
                p.trackContainer->unusedMidiPool.add(tc.linkedMidi);
            }
        }
        else {
            if (tc.linkedAudio) tc.trackPtr->getAudioClips().removeObject(tc.linkedAudio, true);
            if (tc.linkedMidi) tc.trackPtr->getMidiClips().removeObject(tc.linkedMidi, true);
        }

        p.clips.erase(p.clips.begin() + index);
    }

    p.selectedClipIndices.clear();
    p.draggingClipIndex = -1;

    // DOUBLE BUFFER: notificar a los tracks afectados para actualizar sus snapshots.
    // Iteramos sobre los clips eliminados (ya borrados de p.clips) usando un set de tracks.
    // Como ya fueron eliminados, usamos una pass previa para colectar los trackPtrs.
    // Nota: lo más simple es hacer commitSnapshot de TODOS los tracks visibles.
    if (p.tracksRef) {
        for (auto* tr : *p.tracksRef)
            tr->commitSnapshot();
    }

    p.updateScrollBars();
    p.repaint();
    p.hNavigator.repaint(); // SINCRONIZA EL MINIMAPA
}

void PlaylistActionHandler::deleteClipsByName(PlaylistComponent& p, const juce::String& name, bool isMidi) {
    std::unique_ptr<juce::ScopedLock> lock;
    if (p.audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*p.audioMutex);

    for (int i = (int)p.clips.size() - 1; i >= 0; --i) {
        auto& c = p.clips[i];
        if (isMidi && c.linkedMidi && c.linkedMidi->getName() == name) {
            if (p.onMidiClipDeleted) p.onMidiClipDeleted(c.linkedMidi);
            c.trackPtr->getMidiClips().removeObject(c.linkedMidi, true);
            p.clips.erase(p.clips.begin() + i);
        }
        else if (!isMidi && c.linkedAudio && c.linkedAudio->getName() == name) {
            c.trackPtr->getAudioClips().removeObject(c.linkedAudio, true);
            p.clips.erase(p.clips.begin() + i);
        }
    }
    // DOUBLE BUFFER: actualizar snapshots de los tracks afectados
    if (p.tracksRef) {
        for (auto* tr : *p.tracksRef)
            tr->commitSnapshot();
    }
    p.selectedClipIndices.clear();
    p.updateScrollBars();
    p.repaint();
    p.hNavigator.repaint(); // SINCRONIZA EL MINIMAPA
}

void PlaylistActionHandler::purgeClipsOfTrack(PlaylistComponent& p, Track* track) {
    p.clips.erase(std::remove_if(p.clips.begin(), p.clips.end(),
        [track](const TrackClip& c) { return c.trackPtr == track; }),
        p.clips.end());
    p.updateScrollBars();
    p.hNavigator.repaint();
}

void PlaylistActionHandler::handleDoubleClick(PlaylistComponent& p, const juce::MouseEvent& e) {
    if (!p.tracksRef) return;
    int cIdx = p.getClipAt(e.x, e.y);

    if (cIdx != -1 && p.clips[cIdx].linkedMidi != nullptr) {
        if (p.onMidiClipDoubleClicked) p.onMidiClipDoubleClicked(p.clips[cIdx].trackPtr, p.clips[cIdx].linkedMidi);
        return;
    }

    int tIdx = p.getTrackAtY(e.y);
    if (tIdx != -1 && (*p.tracksRef)[tIdx]->getType() == TrackType::MIDI && cIdx == -1) {
        float absX = p.getAbsoluteXFromMouse(e.x);
        float snappedX = std::round(absX / p.snapPixels) * p.snapPixels;

        Track* targetTrack = (*p.tracksRef)[tIdx];
        MidiPattern* newMidiClip = nullptr;

        if (!targetTrack->getMidiClips().isEmpty()) {
            MidiPattern* sourceClip = targetTrack->getMidiClips().getLast();
            newMidiClip = new MidiPattern(*sourceClip);
            newMidiClip->setStartX(snappedX);
            // IMPORTANTE: Ya no sumamos timeShift a las notas. 
            // Las notas son relativas (0-based) para que los clones funcionen.
            newMidiClip->autoVol = sourceClip->autoVol;
            newMidiClip->autoPan = sourceClip->autoPan;
            newMidiClip->autoPitch = sourceClip->autoPitch;
            newMidiClip->autoFilter = sourceClip->autoFilter;
        }
        else {
            int maxPatternNum = 0;
            for (auto* tr : *p.tracksRef) {
                for (auto* mc : tr->getMidiClips()) {
                    if (mc->getName().startsWith("Pattern ")) {
                        int num = mc->getName().substring(8).getIntValue();
                        if (num > maxPatternNum) maxPatternNum = num;
                    }
                }
            }
            newMidiClip = new MidiPattern();
            newMidiClip->setName("Pattern " + juce::String(maxPatternNum + 1));
            newMidiClip->setStartX(snappedX);
            newMidiClip->setWidth(1280.0f);
            newMidiClip->setColor(targetTrack->getColor());
        }

        targetTrack->getMidiClips().add(newMidiClip);
        p.clips.push_back({ targetTrack, snappedX, newMidiClip->getWidth(), newMidiClip->getName(), nullptr, newMidiClip });
        targetTrack->commitSnapshot(); // DOUBLE BUFFER: nuevo clip MIDI creado
        p.updateScrollBars();
        p.repaint();
        p.hNavigator.repaint(); // SINCRONIZA EL MINIMAPA
    }
}