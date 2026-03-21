#pragma once
#include <JuceHeader.h>
#include "Track.h"
#include "TrackControlPanel.h"

class TrackContainer : public juce::Component,
    public juce::DragAndDropTarget,
    public juce::FileDragAndDropTarget {
public:
    std::function<void(Track&, TrackControlPanel&)> onLoadFx;
    std::function<void(Track&, int)> onOpenFx;
    std::function<void(Track&)> onOpenPianoRoll;
    std::function<void(int)> onDeleteTrack;
    std::function<void(Track&)> onOpenEffectsPanel;
    std::function<void()> onTracksReordered;

    int vOffset = 0;

    TrackContainer() {
        // Importante: Agregar el fondo primero para que esté atrás
        addAndMakeVisible(headerBg);
        headerBg.setInterceptsMouseClicks(false, false); // No bloquea clics

        addAndMakeVisible(addMidiBtn);
        addMidiBtn.setButtonText("+ MIDI");
        addMidiBtn.onClick = [this] { addTrack(TrackType::MIDI); };

        addAndMakeVisible(addAudioBtn);
        addAudioBtn.setButtonText("+ AUDIO");
        addAudioBtn.onClick = [this] { addTrack(TrackType::Audio); };
    }

    const juce::OwnedArray<Track>& getTracks() const { return tracks; }

    void addTrack(TrackType type) {
        int id = tracks.size() + 1;
        auto* t = new Track(id, (type == TrackType::MIDI ? "Inst " : "Audio ") + juce::String(id), type);
        tracks.add(t);
        auto* p = new TrackControlPanel(*t);

        p->onFxClick = [this, t, p] { if (onLoadFx) onLoadFx(*t, *p); };
        p->onPluginClick = [this, t](int i) { if (onOpenFx) onOpenFx(*t, i); };
        p->onPianoRollClick = [this, t] { if (onOpenPianoRoll) onOpenPianoRoll(*t); };
        p->onDeleteClick = [this, t] { if (onDeleteTrack) onDeleteTrack(tracks.indexOf(t)); };
        p->onEffectsClick = [this, t] { if (onOpenEffectsPanel) onOpenEffectsPanel(*t); };
        p->onFolderStateChange = [this] {
            recalculateDepthsFromDeltas(); recalculateDeltasFromDepths();
            resized(); repaint(); if (onTracksReordered) onTracksReordered();
            };

        trackPanels.add(p);
        addAndMakeVisible(p);
        recalculateDeltasFromDepths();
        resized();
        if (onTracksReordered) onTracksReordered();
    }

    void removeTrack(int index) {
        if (index >= 0 && index < tracks.size()) {
            trackPanels.remove(index); tracks.remove(index);
            recalculateDeltasFromDepths(); resized(); repaint();
        }
    }

    void setVOffset(int newOffset) {
        vOffset = newOffset;
        resized();
    }

    void recalculateDepthsFromDeltas() {
        int currentDepth = 0;
        for (auto* t : tracks) {
            t->folderDepth = currentDepth;
            if (t->folderDepthChange > 0) currentDepth++;
            else if (t->folderDepthChange < 0) { currentDepth += t->folderDepthChange; if (currentDepth < 0) currentDepth = 0; }
        }
    }

    void recalculateDeltasFromDepths() {
        for (int i = 0; i < tracks.size(); ++i) {
            int currentD = tracks[i]->folderDepth;
            int nextD = (i + 1 < tracks.size()) ? tracks[i + 1]->folderDepth : 0;
            tracks[i]->folderDepthChange = nextD - currentD;
        }

        std::vector<bool> collapseStack;
        for (int i = 0; i < tracks.size(); ++i) {
            auto* t = tracks[i];
            t->isShowingInChildren = true;
            for (bool isC : collapseStack) { if (isC) { t->isShowingInChildren = false; break; } }
            trackPanels[i]->setVisible(t->isShowingInChildren);
            if (t->folderDepthChange == 1) collapseStack.push_back(t->isCollapsed);
            else if (t->folderDepthChange < 0) {
                int drops = -t->folderDepthChange;
                for (int d = 0; d < drops; ++d) { if (!collapseStack.empty()) collapseStack.pop_back(); }
            }
            trackPanels[i]->updateFolderBtnVisuals();
        }
    }

    void refreshTrackPanels() { for (auto* p : trackPanels) p->updatePlugins(); }

    bool isInterestedInDragSource(const SourceDetails& d) override { return d.description == "TRACK"; }
    void itemDragEnter(const SourceDetails&) override {}
    void itemDragExit(const SourceDetails&) override {}
    void itemDragMove(const SourceDetails&) override { repaint(); }
    void itemDropped(const SourceDetails&) override {}

    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void filesDropped(const juce::StringArray& files, int, int) override {
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

    void resized() override {
        auto area = getLocalBounds();
        auto top = area.removeFromTop(35);
        headerBg.setBounds(top);

        addMidiBtn.setBounds(top.removeFromLeft(getWidth() / 2).reduced(2));
        addAudioBtn.setBounds(top.reduced(2));

        int currentY = 35 - vOffset;
        for (auto* p : trackPanels) {
            if (p->isVisible()) {
                p->setBounds(0, currentY, getWidth(), 100);
                currentY += 100;
            }
        }
    }

private:
    juce::Component headerBg;
    juce::TextButton addMidiBtn, addAudioBtn;
    juce::OwnedArray<Track> tracks;
    juce::OwnedArray<TrackControlPanel> trackPanels;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackContainer)
};