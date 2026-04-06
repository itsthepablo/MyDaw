#pragma once
#include <JuceHeader.h>
#include "PlaylistTool.h"
#include "../PlaylistComponent.h"
#include "../../UI/MidiClipRenderer.h"

class PointerTool : public PlaylistTool {
public:
    void mouseDown(const juce::MouseEvent& e, PlaylistComponent& p) override {
        p.grabKeyboardFocus();

        // 1. --- Hit-test para Automation Overlays ---
        if (p.tracksRef) {
            for (auto* t : *p.tracksRef) {
                if (!t->isShowingInChildren) continue;
                for (auto* aut : t->automationClips) {
                    if (aut && aut->isShowing) {
                        int yPos = p.getTrackY(t);
                        if (yPos != -1 && e.y >= yPos && e.y <= yPos + p.trackHeight) {
                            
                            float hS = (float)p.hNavigator.getCurrentRangeStart();
                            float realTime = p.getAbsoluteXFromMouse(e.x);
                            
                            int hitNodeIdx = -1;
                            for (size_t i = 0; i < aut->lane.nodes.size(); ++i) {
                                float screenX = (aut->lane.nodes[i].x * p.hZoom) - hS;
                                float screenY = yPos + p.trackHeight - (aut->lane.nodes[i].value * p.trackHeight);
                                if (std::abs(e.x - screenX) < 10.0f && std::abs(e.y - screenY) < 10.0f) {
                                    hitNodeIdx = (int)i; break;
                                }
                            }

                            int hitTensionNodeIdx = -1;
                            if (hitNodeIdx == -1 && aut->lane.nodes.size() > 1 && p.hoveredAutoClip == aut) {
                                for (size_t i = 0; i < aut->lane.nodes.size() - 1; ++i) {
                                    float midTimeX = (aut->lane.nodes[i].x + aut->lane.nodes[i+1].x) * 0.5f;
                                    float midValY = yPos + p.trackHeight - (aut->lane.getValueAt(midTimeX) * p.trackHeight);
                                    float screenX = (midTimeX * p.hZoom) - hS;
                                    if (std::abs(e.x - screenX) < 10.0f && std::abs(e.y - midValY) < 10.0f) {
                                        hitTensionNodeIdx = (int)i; break;
                                    }
                                }
                            }

                            if (e.mods.isRightButtonDown()) {
                                if (hitNodeIdx != -1) {
                                    aut->lane.nodes.erase(aut->lane.nodes.begin() + hitNodeIdx);
                                    t->commitSnapshot();
                                    p.repaint();
                                }
                                return;
                            }

                            if (e.mods.isLeftButtonDown()) {
                                if (hitTensionNodeIdx != -1) {
                                    p.draggingAutoNode = { aut, -1, hitTensionNodeIdx };
                                    return;
                                }
                                if (hitNodeIdx == -1) {
                                    float val = 1.0f - ((float)(e.y - yPos) / p.trackHeight);
                                    val = juce::jlimit(0.0f, 1.0f, val);
                                    aut->lane.nodes.push_back({ realTime, val, 0.0f, AutomationMath::SingleCurve });
                                    
                                    std::sort(aut->lane.nodes.begin(), aut->lane.nodes.end(), [](const AutoNode& a, const AutoNode& b) { return a.x < b.x; });
                                    
                                    for (size_t i = 0; i < aut->lane.nodes.size(); ++i) {
                                        if (aut->lane.nodes[i].x == realTime && aut->lane.nodes[i].value == val) {
                                            hitNodeIdx = (int)i; break;
                                        }
                                    }
                                    t->commitSnapshot();
                                    p.repaint();
                                }
                                p.draggingAutoNode = { aut, hitNodeIdx, -1 };
                                return;
                            }
                        }
                    }
                }
            }
        }

        int cIdx = p.getClipAt(e.x, e.y);

        if (cIdx != -1) {
            if (p.onClipSelected && p.clips[cIdx].trackPtr != nullptr) p.onClipSelected(p.clips[cIdx].trackPtr);
            
            if (e.mods.isRightButtonDown()) {
                auto* linkedMidi = p.clips[cIdx].linkedMidi;

                juce::PopupMenu m;
                if (linkedMidi != nullptr) {
                    m.addItem(1, "Make Unique");
                    m.addItem(2, "Rename");
                    
                    juce::PopupMenu styleMenu;
                    styleMenu.addItem(10, "Classic (Colored BG)", true, linkedMidi->style == MidiStyleType::Classic);
                    styleMenu.addItem(11, "Modern (Dark BG)", true, linkedMidi->style == MidiStyleType::Modern);
                    styleMenu.addItem(12, "Minimal (Transparent)", true, linkedMidi->style == MidiStyleType::Minimal);
                    styleMenu.addItem(13, "Gradient (Neon Round)", true, linkedMidi->style == MidiStyleType::Gradient);
                    m.addSubMenu("Midi Style", styleMenu);

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
                        targetTrack->commitSnapshot(); // DOUBLE BUFFER: nuevo clip independiente
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
                    else if (result >= 10 && result <= 13 && sourceMidi) {
                        sourceMidi->style = (MidiStyleType)(result - 10);
                        if (targetTrack) targetTrack->commitSnapshot();
                        p.repaint();
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
        // --- Dragging Automatizacion ---
        if (p.draggingAutoNode.clip != nullptr) {
            auto* aut = p.draggingAutoNode.clip;
            Track* targetTrack = nullptr;
            if (p.tracksRef) {
                for (auto* t : *p.tracksRef) {
                    if (t->getId() == aut->targetTrackId) { targetTrack = t; break; }
                }
            }
            if (!targetTrack) return;

            int yPos = p.getTrackY(targetTrack);
            if (yPos == -1) return;

            if (p.draggingAutoNode.tensionNodeIndex != -1) {
                int idx = p.draggingAutoNode.tensionNodeIndex;
                if (idx >= 0 && idx < aut->lane.nodes.size() - 1) {
                    AutoNode& n0 = aut->lane.nodes[idx];
                    AutoNode& n1 = aut->lane.nodes[idx + 1];
                    float vy = 1.0f - ((float)(e.y - yPos) / p.trackHeight);
                    vy = juce::jlimit(0.0f, 1.0f, vy);

                    if (std::abs(n1.value - n0.value) > 0.001f) {
                        float R = (vy - std::min(n0.value, n1.value)) / std::abs(n1.value - n0.value);
                        R = juce::jlimit(0.01f, 0.99f, R);
                        float power = std::log(R) / std::log(0.5f);
                        n0.tension = juce::jlimit(-1.0f, 1.0f, (float)(std::log(power) / std::log(4.0f)));
                    }
                }
                p.repaint();
                targetTrack->commitSnapshot();  
                return;
            }

            if (p.draggingAutoNode.nodeIndex != -1) {
                int idx = p.draggingAutoNode.nodeIndex;
                float realTime = p.getAbsoluteXFromMouse(e.x);
                realTime = juce::jmax(0.0f, realTime);
                
                float val = 1.0f - ((float)(e.y - yPos) / p.trackHeight);
                val = juce::jlimit(0.0f, 1.0f, val);

                aut->lane.nodes[idx].x = realTime;
                aut->lane.nodes[idx].value = val;
                
                auto currentNode = aut->lane.nodes[idx];
                std::sort(aut->lane.nodes.begin(), aut->lane.nodes.end(), [](const AutoNode& a, const AutoNode& b) { return a.x < b.x; });
                for (int i = 0; i < aut->lane.nodes.size(); ++i) {
                    if (std::abs(aut->lane.nodes[i].x - currentNode.x) < 0.00001f && std::abs(aut->lane.nodes[i].value - currentNode.value) < 0.00001f) {
                        p.draggingAutoNode.nodeIndex = i;
                        break;
                    }
                }

                p.repaint();
                targetTrack->commitSnapshot();  
                return;
            }
        }

        if (p.draggingClipIndex == -1) return;

        float absX = p.getAbsoluteXFromMouse(e.x);
        float diff = absX - p.dragStartAbsX;

        if (p.draggingNoteIndex != -1) {
            auto* midiClip = p.clips[p.draggingClipIndex].linkedMidi;
            auto& note = midiClip->notes[p.draggingNoteIndex];
            
            // RELATIVO: snappedX es relativo al inicio del clip (congelado en 0 por ahora para notas)
            float snappedX = std::round((p.dragStartNoteX + diff) / p.snapPixels) * p.snapPixels;

            if (p.isResizingNote) note.width = juce::jmax(10.0f, p.dragStartNoteWidth + diff);
            else note.x = (int)juce::jmax(0.0f, snappedX); // Las notas siempre son >= 0

            p.notifyPatternEdited(midiClip);
            p.repaint();
            return;
        }

        // --- DUPLICACIÓN DE CLIP CON ALT ---
        if (e.mods.isAltDown() && !p.isAltDragging && !p.isResizingClip) {
            p.isAltDragging = true;
            auto& source = p.clips[p.draggingClipIndex];
            
            if (source.linkedMidi) {
                MidiClipData* clone = new MidiClipData(*source.linkedMidi);
                source.trackPtr->midiClips.add(clone);
                p.clips.push_back({ source.trackPtr, source.startX, source.width, source.name, nullptr, clone });
                p.draggingClipIndex = (int)p.clips.size() - 1;
                p.selectedClipIndices.clear();
                p.selectedClipIndices.push_back(p.draggingClipIndex);
            } else if (source.linkedAudio) {
                AudioClipData* clone = new AudioClipData(*source.linkedAudio);
                source.trackPtr->audioClips.add(clone);
                p.clips.push_back({ source.trackPtr, source.startX, source.width, source.name, clone, nullptr });
                p.draggingClipIndex = (int)p.clips.size() - 1;
                p.selectedClipIndices.clear();
                p.selectedClipIndices.push_back(p.draggingClipIndex);
            }
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
            // Las notas MIDI se mantienen relativas al bloque (0-based)
            p.clips[p.draggingClipIndex].startX = snappedX;
            if (p.clips[p.draggingClipIndex].linkedMidi != nullptr) p.clips[p.draggingClipIndex].linkedMidi->startX = snappedX;
        }
        p.repaint();
        p.hNavigator.repaint(); // SINCRONIZA EL MINIMAPA AL ARRASTRAR
    }

    void mouseUp(const juce::MouseEvent& e, PlaylistComponent& p) override {
        // --- Drop Automatizacion ---
        if (p.draggingAutoNode.clip != nullptr) {
            if (p.tracksRef) {
                for (auto* t : *p.tracksRef) {
                    if (t->getId() == p.draggingAutoNode.clip->targetTrackId) {
                        t->commitSnapshot();
                        break;
                    }
                }
            }
            p.draggingAutoNode = { nullptr, -1 };
            return;
        }

        if (p.draggingClipIndex != -1) {
            auto& clip = p.clips[p.draggingClipIndex];

            if (p.draggingNoteIndex == -1) {
                // Fin de movimiento/resize de clip
                if (p.isResizingClip) {
                    if (clip.linkedAudio != nullptr) clip.linkedAudio->width = clip.width;
                }
                else {
                    if (clip.linkedAudio != nullptr) clip.linkedAudio->startX = clip.startX;
                }
                // DOUBLE BUFFER: publicar la posición/tamaño final al audio thread
                if (clip.trackPtr != nullptr)
                    clip.trackPtr->commitSnapshot();
            }
            else {
                // Fin de edición inline de nota MIDI
                auto* midiClip = clip.linkedMidi;
                if (midiClip != nullptr && clip.trackPtr != nullptr)
                    clip.trackPtr->commitSnapshot(); // DOUBLE BUFFER: nota movida/resizada
            }
        }

        p.draggingClipIndex = -1;
        p.draggingNoteIndex = -1;
        p.isAltDragging = false;
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