#pragma once
#include <JuceHeader.h>
#include "PlaylistTool.h"
#include "../PlaylistComponent.h"
#include "../../UI/MidiClipRenderer.h"

class PointerTool : public PlaylistTool {
public:
    void mouseDown(const juce::MouseEvent& e, PlaylistComponent& p) override {
        p.grabKeyboardFocus();

        int cIdx = p.getClipAt(e.x, e.y);

        if (cIdx != -1) {
            if (p.onClipSelected && p.clips[cIdx].trackPtr != nullptr) p.onClipSelected(p.clips[cIdx].trackPtr);
            
            if (e.mods.isRightButtonDown()) {
                auto* linkedMidi = p.clips[cIdx].linkedMidi;

                juce::PopupMenu m;
                if (linkedMidi != nullptr) {
                    m.addItem(1, "Make Unique");
                    m.addItem(2, "Rename");
                    m.addSeparator();
                }
                m.addItem(3, "Eliminar Clip");

                m.showMenuAsync(juce::PopupMenu::Options(), [&p, cIdx](int result) {
                    if (result == 0) return;
                    if (result == 3) { 
                        if (std::find(p.selectedClipIndices.begin(), p.selectedClipIndices.end(), cIdx) == p.selectedClipIndices.end()) {
                            p.selectedClipIndices.clear();
                            p.selectedClipIndices.push_back(cIdx);
                        }
                        p.deleteSelectedClips(); 
                        return; 
                    }
                    if (cIdx >= p.clips.size()) return;

                    MidiClipData* sourceMidi = p.clips[cIdx].linkedMidi;
                    Track* targetTrack = p.clips[cIdx].trackPtr;

                    if (result == 1 && sourceMidi && targetTrack) {
                        MidiClipData* newMidiClip = new MidiClipData(*sourceMidi);
                        int maxPatternNum = 0;
                        if (p.tracksRef) {
                            for (auto* tr : *p.tracksRef) {
                                for (auto* mc : tr->midiClips) {
                                    if (mc->name.startsWith("Pattern ")) {
                                        int num = mc->name.substring(8).getIntValue();
                                        if (num > maxPatternNum) maxPatternNum = num;
                                    }
                                }
                            }
                        }
                        newMidiClip->name = "Pattern " + juce::String(maxPatternNum + 1);
                        targetTrack->midiClips.add(newMidiClip);
                        p.clips[cIdx].linkedMidi = newMidiClip;
                        p.clips[cIdx].name = newMidiClip->name;
                        p.repaint();
                        p.hNavigator.repaint();
                    }
                    else if (result == 2 && sourceMidi) {
                        juce::MessageManager::callAsync([&p, sourceMidi]() {
                            auto* alert = new juce::AlertWindow("Rename Pattern", "Enter new name:", juce::AlertWindow::QuestionIcon);
                            alert->addTextEditor("newName", sourceMidi->name);
                            alert->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey, 0, 0));
                            alert->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey, 0, 0));

                            alert->enterModalState(false, juce::ModalCallbackFunction::create([&p, sourceMidi, alert](int btn) {
                                alert->setVisible(false);
                                if (btn == 1) {
                                    juce::String newName = alert->getTextEditorContents("newName");
                                    juce::String oldName = sourceMidi->name;

                                    if (newName.isNotEmpty() && newName != oldName) {
                                        if (p.tracksRef) {
                                            for (auto* tr : *p.tracksRef) {
                                                for (auto* mc : tr->midiClips) { if (mc->name == oldName) mc->name = newName; }
                                            }
                                        }
                                        for (auto& cv : p.clips) { if (cv.name == oldName) cv.name = newName; }
                                        p.repaint();
                                        p.hNavigator.repaint();
                                    }
                                }
                                juce::MessageManager::callAsync([alert]() { delete alert; });
                                }));
                            });
                    }
                    });
                return;
            }

            auto& clip = p.clips[cIdx];
            float hS = (float)p.hNavigator.getCurrentRangeStart();

            if (clip.trackPtr->isInlineEditingActive && clip.linkedMidi) {
                int yPos = p.getTrackY(clip.trackPtr);
                int xPos = (int)(clip.startX * p.hZoom) - (int)hS;
                int wPos = (int)(clip.width * p.hZoom);
                juce::Rectangle<int> clipRect(xPos, yPos + 2, wPos - 1, (int)p.trackHeight - 4);

                int noteIdx = MidiClipRenderer::hitTestInlineNote(*clip.linkedMidi, clipRect, e.x, e.y, p.hZoom, hS);

                if (noteIdx != -1) {
                    auto& note = clip.linkedMidi->notes[noteIdx];
                    int noteScreenX = (int)(note.x * p.hZoom) - (int)hS;
                    int noteScreenW = std::max(3, (int)(note.width * p.hZoom));

                    p.draggingClipIndex = cIdx;
                    p.draggingNoteIndex = noteIdx;
                    p.dragStartAbsX = p.getAbsoluteXFromMouse(e.x);
                    p.dragStartNoteX = note.x;
                    p.dragStartNoteWidth = note.width;
                    p.isResizingNote = e.x > (noteScreenX + noteScreenW - 6);
                    p.repaint();
                    return;
                }
            }

            p.draggingClipIndex = cIdx;
            p.draggingNoteIndex = -1;
            p.dragStartAbsX = p.getAbsoluteXFromMouse(e.x);
            p.dragStartXOriginal = p.clips[cIdx].startX;
            p.dragStartWidth = p.clips[cIdx].width;

            float clipScreenX = (p.clips[cIdx].startX * p.hZoom) - hS;
            float clipScreenW = p.clips[cIdx].width * p.hZoom;
            p.isResizingClip = e.x > (clipScreenX + clipScreenW - 10);
            
            // Multiselection Logic:
            if (e.mods.isCommandDown() || e.mods.isShiftDown()) {
                auto it = std::find(p.selectedClipIndices.begin(), p.selectedClipIndices.end(), cIdx);
                if (it == p.selectedClipIndices.end()) p.selectedClipIndices.push_back(cIdx);
                else p.selectedClipIndices.erase(it);
            } else {
                if (std::find(p.selectedClipIndices.begin(), p.selectedClipIndices.end(), cIdx) == p.selectedClipIndices.end()) {
                    p.selectedClipIndices.clear();
                    p.selectedClipIndices.push_back(cIdx);
                }
            }
        }
        else {
            p.draggingClipIndex = -1;
            p.draggingNoteIndex = -1;
            if (!e.mods.isCommandDown() && !e.mods.isShiftDown()) {
               p.selectedClipIndices.clear();
            }
        }
        p.repaint();
    }

    void mouseDrag(const juce::MouseEvent& e, PlaylistComponent& p) override {
        if (p.draggingClipIndex == -1) return;

        float absX = p.getAbsoluteXFromMouse(e.x);
        float diff = absX - p.dragStartAbsX;

        if (p.draggingNoteIndex != -1) {
            auto* midiClip = p.clips[p.draggingClipIndex].linkedMidi;
            auto& note = midiClip->notes[p.draggingNoteIndex];
            float snappedX = std::round((p.dragStartNoteX + diff) / p.snapPixels) * p.snapPixels;

            if (p.isResizingNote) note.width = juce::jmax(10.0f, p.dragStartNoteWidth + diff);
            else note.x = juce::jmax(midiClip->startX, snappedX);

            p.notifyPatternEdited(midiClip);
            p.repaint();
            return;
        }

        int newTrackIdx = p.getTrackAtY(e.y);
        if (newTrackIdx != -1 && (*p.tracksRef)[newTrackIdx] != p.clips[p.draggingClipIndex].trackPtr && !p.isResizingClip) {
            Track* newTrack = (*p.tracksRef)[newTrackIdx];
            Track* oldTrack = p.clips[p.draggingClipIndex].trackPtr;

            bool isAudio = p.clips[p.draggingClipIndex].linkedAudio != nullptr;
            bool isMidi = p.clips[p.draggingClipIndex].linkedMidi != nullptr;

            if ((isAudio && newTrack->getType() == TrackType::Audio) || (isMidi && newTrack->getType() == TrackType::MIDI)) {
                std::unique_ptr<juce::ScopedLock> lock;
                if (p.audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*p.audioMutex);

                if (isMidi) {
                    auto* m = p.clips[p.draggingClipIndex].linkedMidi;
                    oldTrack->midiClips.removeObject(m, false);
                    newTrack->midiClips.add(m);
                }
                else if (isAudio) {
                    auto* a = p.clips[p.draggingClipIndex].linkedAudio;
                    oldTrack->audioClips.removeObject(a, false);
                    newTrack->audioClips.add(a);
                }
                p.clips[p.draggingClipIndex].trackPtr = newTrack;
            }
        }

        float snappedX = std::round((p.dragStartXOriginal + diff) / p.snapPixels) * p.snapPixels;
        snappedX = juce::jmax(0.0f, snappedX);

        if (p.isResizingClip) {
            float newW = juce::jmax(10.0f, p.dragStartWidth + diff);
            p.clips[p.draggingClipIndex].width = newW;
            // No actualizar linkedAudio en tiempo real para evitar glitcheo de lectura
            if (p.clips[p.draggingClipIndex].linkedMidi != nullptr) p.clips[p.draggingClipIndex].linkedMidi->width = newW;
        }
        else {
            if (p.clips[p.draggingClipIndex].linkedMidi != nullptr) {
                float timeShift = snappedX - p.clips[p.draggingClipIndex].linkedMidi->startX;
                for (auto& note : p.clips[p.draggingClipIndex].linkedMidi->notes) note.x += timeShift;
                auto shiftAutoLane = [timeShift](AutoLane& lane) { for (auto& node : lane.nodes) node.x += timeShift; };
                shiftAutoLane(p.clips[p.draggingClipIndex].linkedMidi->autoVol);
                shiftAutoLane(p.clips[p.draggingClipIndex].linkedMidi->autoPan);
                shiftAutoLane(p.clips[p.draggingClipIndex].linkedMidi->autoPitch);
                shiftAutoLane(p.clips[p.draggingClipIndex].linkedMidi->autoFilter);
            }

            p.clips[p.draggingClipIndex].startX = snappedX;
            // Retenemos el linkedAudio->startX original hasta soltar el clic.
            if (p.clips[p.draggingClipIndex].linkedMidi != nullptr) p.clips[p.draggingClipIndex].linkedMidi->startX = snappedX;
        }
        p.repaint();
        p.hNavigator.repaint(); // SINCRONIZA EL MINIMAPA AL ARRASTRAR
    }

    void mouseUp(const juce::MouseEvent& e, PlaylistComponent& p) override {
        if (p.draggingClipIndex != -1 && p.draggingNoteIndex == -1) {
            auto& clip = p.clips[p.draggingClipIndex];

            if (p.isResizingClip) {
                if (clip.linkedAudio != nullptr) clip.linkedAudio->width = clip.width;
            }
            else {
                if (clip.linkedAudio != nullptr) clip.linkedAudio->startX = clip.startX;
            }
        }
        
        p.draggingClipIndex = -1;
        p.draggingNoteIndex = -1;
    }

    void mouseMove(const juce::MouseEvent& e, PlaylistComponent& p) override {
        int idx = p.getClipAt(e.x, e.y);
        if (idx != -1) {
            auto& clip = p.clips[idx];
            float hS = (float)p.hNavigator.getCurrentRangeStart();

            if (clip.trackPtr->isInlineEditingActive && clip.linkedMidi) {
                int yPos = p.getTrackY(clip.trackPtr);
                int xPos = (int)(clip.startX * p.hZoom) - (int)hS;
                int wPos = (int)(clip.width * p.hZoom);
                juce::Rectangle<int> clipRect(xPos, yPos + 2, wPos - 1, (int)p.trackHeight - 4);

                int noteIdx = MidiClipRenderer::hitTestInlineNote(*clip.linkedMidi, clipRect, e.x, e.y, p.hZoom, hS);

                if (noteIdx != -1) {
                    auto& note = clip.linkedMidi->notes[noteIdx];
                    int noteScreenX = (int)(note.x * p.hZoom) - (int)hS;
                    int noteScreenW = std::max(3, (int)(note.width * p.hZoom));

                    if (e.x > noteScreenX + noteScreenW - 6) p.setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
                    else p.setMouseCursor(juce::MouseCursor::DraggingHandCursor);
                    return;
                }
            }

            float edgeX = (clip.startX * p.hZoom) - hS + (clip.width * p.hZoom);
            if (e.x > edgeX - 10) p.setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
            else p.setMouseCursor(juce::MouseCursor::DraggingHandCursor);
        }
        else {
            p.setMouseCursor(juce::MouseCursor::NormalCursor);
        }
    }
};