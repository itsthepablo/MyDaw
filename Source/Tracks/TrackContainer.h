#pragma once
#include <JuceHeader.h>
#include "Track.h"
#include "TrackControlPanel.h"
#include <vector>
#include <algorithm>

struct TrackHeaderBackground : public juce::Component {
    TrackHeaderBackground() { setOpaque(true); }
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(25, 27, 30));
    }
};

class TrackContainer : public juce::Component,
    public juce::DragAndDropTarget,
    public juce::FileDragAndDropTarget {
public:
    std::function<void(Track&, TrackControlPanel&)> onLoadFx;
    std::function<void(Track&)> onOpenInstrument; // NUEVO
    std::function<void(Track&, int)> onOpenFx;
    std::function<void(Track&)> onOpenPianoRoll;
    std::function<void(int)> onDeleteTrack;
    std::function<void(Track&)> onOpenEffects;
    std::function<void()> onTracksReordered;
    std::function<void()> onTrackAdded;
    std::function<void(Track*)> onActiveTrackChanged;
    std::function<void(float)> onScrollWheel;

    int vOffset = 0;
    float trackHeight = 100.0f;

    void setTrackHeight(float newHeight) {
        trackHeight = newHeight;
        resized();
        repaint();
    }

    // --- NUEVO: POOL DE RECICLAJE (Clips no utilizados) ---
    juce::OwnedArray<AudioClipData> unusedAudioPool;
    juce::OwnedArray<MidiClipData> unusedMidiPool;

    TrackContainer() {
        addAndMakeVisible(headerBg);
        headerBg.setInterceptsMouseClicks(true, false);

        addAndMakeVisible(addMidiBtn);
        addMidiBtn.setButtonText("+ MIDI");
        addMidiBtn.onClick = [this] { addTrack(TrackType::MIDI); };

        addAndMakeVisible(addAudioBtn);
        addAudioBtn.setButtonText("+ AUDIO");
        addAudioBtn.onClick = [this] { addTrack(TrackType::Audio); };
    }

    void setExternalMutex(juce::CriticalSection* mutex) { audioMutex = mutex; }

    const juce::OwnedArray<Track>& getTracks() const { return tracks; }

    void selectTrack(Track* selectedTrack, const juce::ModifierKeys& mods) {
        int clickedIndex = tracks.indexOf(selectedTrack);
        if (clickedIndex < 0) return;

        if (mods.isCommandDown() || mods.isCtrlDown()) {
            selectedTrack->isSelected = !selectedTrack->isSelected;
            lastSelectedTrackIndex = clickedIndex;
        }
        else if (mods.isShiftDown()) {
            if (lastSelectedTrackIndex >= 0 && lastSelectedTrackIndex < tracks.size()) {
                int start = std::min(lastSelectedTrackIndex, clickedIndex);
                int end = std::max(lastSelectedTrackIndex, clickedIndex);
                for (int i = start; i <= end; ++i) tracks[i]->isSelected = true;
            }
            else {
                selectedTrack->isSelected = true;
                lastSelectedTrackIndex = clickedIndex;
            }
        }
        else {
            for (auto* t : tracks) t->isSelected = false;
            selectedTrack->isSelected = true;
            lastSelectedTrackIndex = clickedIndex;
        }

        for (auto* p : trackPanels) p->repaint();
        if (onActiveTrackChanged) onActiveTrackChanged(selectedTrack);
    }

    void addTrack(TrackType type) {
        std::unique_ptr<juce::ScopedLock> lock;
        if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

        int id = (int)tracks.size() + 1;
        auto* t = new Track(id, (type == TrackType::MIDI ? "Inst " : "Audio ") + juce::String(id), type);
        tracks.add(t);
        auto* p = new TrackControlPanel(*t);

        p->onFxClick = [this, t, p] { if (onLoadFx) onLoadFx(*t, *p); };
        p->onInstrumentClick = [this, t] { if (onOpenInstrument) onOpenInstrument(*t); }; // NUEVO
        p->onPluginClick = [this, t](int i) { if (onOpenFx) onOpenFx(*t, i); };
        p->onPianoRollClick = [this, t] { if (onOpenPianoRoll) onOpenPianoRoll(*t); };
        p->onDeleteClick = [this, t] { 
            if (t->isSelected) {
                for (int i = tracks.size() - 1; i >= 0; --i) {
                    if (tracks[i]->isSelected && onDeleteTrack) onDeleteTrack(i);
                }
            } else {
                if (onDeleteTrack) onDeleteTrack(tracks.indexOf(t)); 
            }
        };
        p->onEffectsClick = [this, t] { if (onOpenEffects) onOpenEffects(*t); };

        p->onTrackSelected = [this, t](const juce::ModifierKeys& mods) { selectTrack(t, mods); };

        p->onFolderStateChange = [this] {
            recalculateFolderHierarchy();
            resized(); repaint(); if (onTracksReordered) onTracksReordered();
            };

        p->onWaveformViewChanged = [this] {
            repaint();
            if (getParentComponent()) getParentComponent()->repaint();
            };

        p->onTrackColorChanged = [this, t] {
            if (t->isSelected) {
                juce::Colour nc = t->getColor();
                for (auto* trk : tracks) {
                    if (trk->isSelected) trk->setColor(nc);
                }
            }
            repaint();
            if (getParentComponent()) getParentComponent()->repaint();
            };

        trackPanels.add(p);
        addAndMakeVisible(p);
        recalculateFolderHierarchy();
        resized();

        if (onTracksReordered) onTracksReordered();
        if (onTrackAdded) onTrackAdded();
    }

    // --- MODIFICADO: Salvar clips al pool antes de destruir el track ---
    void removeTrack(int index) {
        std::unique_ptr<juce::ScopedLock> lock;
        if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

        if (index >= 0 && index < tracks.size()) {
            auto* t = tracks[index];

            // Rescatamos los clips y los pasamos al Pool de reciclaje para no perderlos
            for (int i = t->audioClips.size() - 1; i >= 0; --i) {
                auto* a = t->audioClips[i];
                t->audioClips.removeObject(a, false); // false = no destruir de la memoria
                unusedAudioPool.add(a);
            }
            for (int i = t->midiClips.size() - 1; i >= 0; --i) {
                auto* m = t->midiClips[i];
                t->midiClips.removeObject(m, false);
                unusedMidiPool.add(m);
            }

            trackPanels.remove(index);
            tracks.remove(index);

            if (index == lastSelectedTrackIndex) lastSelectedTrackIndex = -1;
            else if (index < lastSelectedTrackIndex) lastSelectedTrackIndex--;

            recalculateFolderHierarchy(); resized(); repaint();
        }

        if (tracks.isEmpty() && onActiveTrackChanged) onActiveTrackChanged(nullptr);
    }

    // --- NUEVO: Borrado permanente invocado por el Picker ---
    void deleteUnusedClipsByName(const juce::String& name, bool isMidi) {
        std::unique_ptr<juce::ScopedLock> lock;
        if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

        if (isMidi) {
            for (int i = unusedMidiPool.size() - 1; i >= 0; --i) {
                if (unusedMidiPool[i]->name == name) unusedMidiPool.remove(i, true);
            }
        }
        else {
            for (int i = unusedAudioPool.size() - 1; i >= 0; --i) {
                if (unusedAudioPool[i]->name == name) unusedAudioPool.remove(i, true);
            }
        }
    }

    void setVOffset(int newOffset) {
        vOffset = newOffset;
        resized();
    }

    void recalculateFolderHierarchy() {
        int currentDepth = 0;
        std::vector<bool> collapseStack;

        for (int i = 0; i < tracks.size(); ++i) {
            auto* t = tracks[i];
            t->folderDepth = currentDepth;
            t->isShowingInChildren = true;
            for (bool isCollapsed : collapseStack) {
                if (isCollapsed) { t->isShowingInChildren = false; break; }
            }
            trackPanels[i]->setVisible(t->isShowingInChildren);

            if (t->isFolderStart) {
                currentDepth++;
                collapseStack.push_back(t->isCollapsed);
            }

            if (t->isFolderEnd) {
                currentDepth--;
                if (currentDepth < 0) currentDepth = 0;
                if (!collapseStack.empty()) collapseStack.pop_back();
            }
            trackPanels[i]->updateFolderBtnVisuals();
        }
    }

    void refreshTrackPanels() { for (auto* p : trackPanels) p->updatePlugins(); }

    bool isInterestedInDragSource(const SourceDetails& d) override { return d.description == "TRACK"; }

    void itemDragMove(const SourceDetails& d) override {
        if (d.description != "TRACK") return;
        auto* draggedPanel = dynamic_cast<TrackControlPanel*>(d.sourceComponent.get());
        if (!draggedPanel) return;

        draggedPanelForGhost = draggedPanel;
        if (ghostSnapshot.isNull()) ghostSnapshot = draggedPanel->createComponentSnapshot(draggedPanel->getLocalBounds());
        ghostY = d.localPosition.y - (draggedPanel->getHeight() / 2);

        for (auto* p : trackPanels) {
            if (!p->isVisible()) continue;
            int oldMode = p->dragHoverMode;
            if (p->getBounds().contains(d.localPosition) && p != draggedPanel) {
                float normY = (float)(d.localPosition.y - p->getY()) / (float)p->getHeight();
                if (normY < 0.25f) p->dragHoverMode = 1;
                else if (normY > 0.75f) {
                    if (p->track.folderDepth > 0 && d.localPosition.x < 40) p->dragHoverMode = 4;
                    else p->dragHoverMode = 3;
                }
                else p->dragHoverMode = 2;
            }
            else {
                p->dragHoverMode = 0;
            }
            if (oldMode != p->dragHoverMode) p->repaint();
        }
        repaint();
    }

    void itemDragExit(const SourceDetails& d) override {
        if (d.description != "TRACK") return;
        draggedPanelForGhost = nullptr;
        ghostSnapshot = juce::Image();
        for (auto* p : trackPanels) {
            if (p->dragHoverMode != 0) { p->dragHoverMode = 0; p->repaint(); }
        }
        repaint();
    }

    void itemDropped(const SourceDetails& d) override {
        if (d.description != "TRACK") return;

        draggedPanelForGhost = nullptr;
        ghostSnapshot = juce::Image();

        auto* draggedPanel = dynamic_cast<TrackControlPanel*>(d.sourceComponent.get());
        if (!draggedPanel) return;

        int sourceIndex = tracks.indexOf(&draggedPanel->track);
        if (sourceIndex < 0) return;

        int targetIndex = -1;
        int mode = 0;

        for (int i = 0; i < trackPanels.size(); ++i) {
            auto* p = trackPanels[i];
            if (p->isVisible() && p->getBounds().contains(d.localPosition)) {
                targetIndex = i;
                float normY = (float)(d.localPosition.y - p->getY()) / (float)p->getHeight();

                if (normY < 0.25f) mode = 1;
                else if (normY > 0.75f) {
                    if (p->track.folderDepth > 0 && d.localPosition.x < 40) mode = 4;
                    else mode = 3;
                }
                else mode = 2;
                break;
            }
        }

        for (auto* p : trackPanels) { p->dragHoverMode = 0; p->repaint(); }

        if (targetIndex >= 0 && targetIndex != sourceIndex) {
            std::unique_ptr<juce::ScopedLock> lock;
            if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

            bool wasFolderEnd = tracks[sourceIndex]->isFolderEnd;

            auto trackToMove = tracks.removeAndReturn(sourceIndex);
            auto panelToMove = trackPanels.removeAndReturn(sourceIndex);

            if (wasFolderEnd && sourceIndex > 0) tracks[sourceIndex - 1]->isFolderEnd = true;
            trackToMove->isFolderEnd = false;

            if (sourceIndex < targetIndex) targetIndex--;

            if (mode == 1) {
                tracks.insert(targetIndex, trackToMove);
                trackPanels.insert(targetIndex, panelToMove);
            }
            else if (mode == 3) {
                tracks.insert(targetIndex + 1, trackToMove);
                trackPanels.insert(targetIndex + 1, panelToMove);
            }
            else if (mode == 2) {
                bool wasFolder = tracks[targetIndex]->isFolderStart;
                tracks[targetIndex]->isFolderStart = true;
                if (!wasFolder) trackToMove->isFolderEnd = true;
                tracks.insert(targetIndex + 1, trackToMove);
                trackPanels.insert(targetIndex + 1, panelToMove);
            }
            else if (mode == 4) {
                tracks[targetIndex]->isFolderEnd = true;
                tracks.insert(targetIndex + 1, trackToMove);
                trackPanels.insert(targetIndex + 1, panelToMove);
            }

            lastSelectedTrackIndex = -1;
            recalculateFolderHierarchy();
            resized();
            repaint();
            if (onTracksReordered) onTracksReordered();
        }
        else {
            repaint();
        }
    }

    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }

    void filesDropped(const juce::StringArray& files, int, int) override {
        std::unique_ptr<juce::ScopedLock> lock;
        if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

        for (auto path : files) {
            juce::File f(path);
            if (f.getFileExtension().containsIgnoreCase("wav")) {
                addTrack(TrackType::Audio);
                tracks.getLast()->setName(f.getFileNameWithoutExtension());
            }
        }
        resized();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(25, 27, 30));
    }

    void paintOverChildren(juce::Graphics& g) override {
        if (draggedPanelForGhost != nullptr && !ghostSnapshot.isNull()) {
            g.setOpacity(0.6f);
            g.drawImageAt(ghostSnapshot, 0, ghostY);
        }
    }

    void resized() override {
        auto area = getLocalBounds();
        auto top = area.removeFromTop(120);
        headerBg.setBounds(top);

        auto buttonArea = top.withTrimmedTop(120 - 35);
        addMidiBtn.setBounds(buttonArea.removeFromLeft(getWidth() / 2).reduced(2));
        addAudioBtn.setBounds(buttonArea.reduced(2));

        int currentY = 120 - vOffset;
        for (auto* p : trackPanels) {
            if (p->isVisible()) {
                p->setBounds(0, currentY, getWidth(), (int)trackHeight);
                currentY += (int)trackHeight;
            }
        }

        headerBg.toFront(false);
        addMidiBtn.toFront(false);
        addAudioBtn.toFront(false);
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override {
        if (onScrollWheel) onScrollWheel(wheel.deltaY);
        juce::Component::mouseWheelMove(e, wheel);
    }

private:
    TrackHeaderBackground headerBg;
    juce::TextButton addMidiBtn, addAudioBtn;
    juce::OwnedArray<Track> tracks;
    juce::OwnedArray<TrackControlPanel> trackPanels;

    juce::CriticalSection* audioMutex = nullptr;

    TrackControlPanel* draggedPanelForGhost = nullptr;
    int ghostY = 0;
    juce::Image ghostSnapshot;

    int lastSelectedTrackIndex = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackContainer)
};