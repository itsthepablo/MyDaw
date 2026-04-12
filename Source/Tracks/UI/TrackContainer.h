#pragma once
#include <JuceHeader.h>
#include "../../Theme/CustomTheme.h"
#include "../../Data/Track.h"
#include "TrackControlPanel.h"
#include "MasterTrackStrip.h"
#include <vector>
#include <algorithm>
#include <map>

struct TrackHeaderBackground : public juce::Component {
    TrackHeaderBackground() { setOpaque(true); }
    void paint(juce::Graphics& g) override {
        if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
            g.fillAll(theme->getSkinColor("TRACKS_BG", juce::Colour(25, 27, 30)));
        } else {
            g.fillAll(juce::Colour(25, 27, 30));
        }
    }
};

class TrackContainer : public juce::Component,
    public juce::DragAndDropTarget,
    public juce::FileDragAndDropTarget {
public:
    std::function<void(Track&, TrackControlPanel&)> onLoadFx;
    std::function<void(Track&)> onOpenInstrument; 
    std::function<void(Track&, int)> onOpenFx;
    std::function<void(Track&)> onOpenPianoRoll;
    std::function<void(int)> onDeleteTrack;
    std::function<void(Track&)> onOpenEffects;
    std::function<void()> onTracksReordered;
    std::function<void()> onTrackAdded;
    std::function<void(Track*)> onActiveTrackChanged;
    std::function<void(float)> onScrollWheel;
    std::function<void(TrackType, bool)> onToggleAnalysisTrack;
    std::function<void()> onDeselectMasterRequested;
    
    int vOffset = 0;
    float trackHeight = 100.0f;

    void setTrackHeight(float newHeight) {
        trackHeight = newHeight;
        resized();
        repaint();
    }

    juce::OwnedArray<AudioClip> unusedAudioPool;
    juce::OwnedArray<MidiPattern> unusedMidiPool;
    juce::OwnedArray<AutomationClipData> automationPool;

    // --- NUEVO: Contenedor para CLIPPING automático ---
    struct TrackContent : public juce::Component {
        TrackContent() { setInterceptsMouseClicks(false, true); }
    } trackContent;

    TrackContainer() {
        addAndMakeVisible(trackContent);
        
        addAndMakeVisible(headerBg);
        headerBg.setInterceptsMouseClicks(true, false);

        addAndMakeVisible(addMidiBtn);
        addAndMakeVisible(addAudioBtn);

        addMidiBtn.onClick = [this] { addTrack(TrackType::MIDI); };
        addAudioBtn.onClick = [this] { addTrack(TrackType::Audio); };

        // --- Botones de Análisis (Switches) ---
        addAndMakeVisible(toggleLdnBtn);
        addAndMakeVisible(toggleBalBtn);
        addAndMakeVisible(toggleMsBtn);

        toggleLdnBtn.setClickingTogglesState(true);
        toggleBalBtn.setClickingTogglesState(true);
        toggleMsBtn.setClickingTogglesState(true);

        toggleLdnBtn.onClick = [this] { if (onToggleAnalysisTrack) onToggleAnalysisTrack(TrackType::Loudness, toggleLdnBtn.getToggleState()); };
        toggleBalBtn.onClick = [this] { if (onToggleAnalysisTrack) onToggleAnalysisTrack(TrackType::Balance, toggleBalBtn.getToggleState()); };
        toggleMsBtn.onClick = [this] { if (onToggleAnalysisTrack) onToggleAnalysisTrack(TrackType::MidSide, toggleMsBtn.getToggleState()); };
    }

    void setExternalMutex(juce::CriticalSection* mutex) { audioMutex = mutex; }

    const juce::OwnedArray<Track>& getTracks() const { return tracks; }

    void deselectAllTracks() {
        for (auto* t : tracks) t->isSelected = false;
        for (auto* p : trackPanels) p->repaint();
    }

    void selectTrack(Track* selectedTrack, const juce::ModifierKeys& mods) {
        int clickedIndex = tracks.indexOf(selectedTrack);
        if (clickedIndex < 0) return;

        if (mods.isCommandDown() || mods.isCtrlDown()) {
            selectedTrack->isSelected = !selectedTrack->isSelected;
            lastSelectedTrackIndex = clickedIndex;
            if (selectedTrack->isSelected && onDeselectMasterRequested) onDeselectMasterRequested();
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
            if (onDeselectMasterRequested) onDeselectMasterRequested();
        }
        else {
            for (auto* t : tracks) t->isSelected = false;
            selectedTrack->isSelected = true;
            lastSelectedTrackIndex = clickedIndex;
            if (onDeselectMasterRequested) onDeselectMasterRequested();
        }

        for (auto* p : trackPanels) p->repaint();
        if (onActiveTrackChanged) onActiveTrackChanged(selectedTrack);
    }

    Track* addTrack(TrackType type, int idToUse = -1) {
        std::unique_ptr<juce::ScopedLock> lock;
        if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

        int id = idToUse > 0 ? idToUse : nextTrackId++;
        
        // Sincronizar el contador si estamos forzando un ID (ej. al cargar proyecto)
        if (idToUse >= nextTrackId) nextTrackId = idToUse + 1;

        juce::String nameToUse;
        if (type == TrackType::MIDI) nameToUse = "Inst " + juce::String(id);
        else if (type == TrackType::Audio) nameToUse = "Audio " + juce::String(id);
        else if (type == TrackType::Folder) nameToUse = "Folder " + juce::String(id);
        else if (type == TrackType::Loudness) nameToUse = "Loudness Track";
        else if (type == TrackType::Balance) nameToUse = "Balance Track";
        else if (type == TrackType::MidSide) nameToUse = "Mid-Side Track";
        else nameToUse = "Track " + juce::String(id);

        auto* t = new Track(id, nameToUse, type);
        tracks.add(t);
        auto* p = new TrackControlPanel(*t);

        p->onFxClick = [this, t, p] { if (onLoadFx) onLoadFx(*t, *p); };
        p->onInstrumentClick = [this, t] { if (onOpenInstrument) onOpenInstrument(*t); };
        p->onPluginClick = [this, t](int i) { if (onOpenFx) onOpenFx(*t, i); };
        p->onPianoRollClick = [this, t] { if (onOpenPianoRoll) onOpenPianoRoll(*t); };
        p->onDeleteClick = [this, t] {
            juce::MessageManager::callAsync([this, t] {
                if (!tracks.contains(t)) return;
                if (t->isSelected) {
                    std::vector<int> toDelete;
                    for (int i = 0; i < tracks.size(); ++i) if (tracks[i]->isSelected) toDelete.push_back(i);
                    for (int i = (int)toDelete.size() - 1; i >= 0; --i) if (onDeleteTrack) onDeleteTrack(toDelete[i]);
                } else {
                    if (onDeleteTrack) onDeleteTrack(tracks.indexOf(t));
                }
            });
        };
        p->onEffectsClick = [this, t] { if (onOpenEffects) onOpenEffects(*t); };
        p->onTrackSelected = [this, t](const juce::ModifierKeys& mods) { selectTrack(t, mods); };
        p->onMoveToNewFolder = [this] { moveSelectedTracksToNewFolder(); };
        p->onFolderStateChange = [this] { recalculateFolderHierarchy(); resized(); repaint(); if (onTracksReordered) onTracksReordered(); };
        p->onWaveformViewChanged = [this] { repaint(); if (getParentComponent()) getParentComponent()->repaint(); };
        p->onTrackColorChanged = [this, t] {
            if (t->isSelected) {
                juce::Colour nc = t->getColor();
                for (auto* trk : tracks) if (trk->isSelected) trk->setColor(nc);
            }
            repaint(); if (getParentComponent()) getParentComponent()->repaint();
        };

        p->onCreateAutomation = [this, t](int paramId, juce::String paramName) {
            createAutomation(t->getId(), paramId, paramName);
        };

        trackPanels.add(p);
        trackContent.addAndMakeVisible(p); // AÑADIDO AL CONTENEDOR DE CLIPPING
        recalculateFolderHierarchy();
        resized();
        if (onTracksReordered) onTracksReordered();
        if (onTrackAdded) onTrackAdded();
        return t;
    }

    Track* getMasterTrack() {
        for (auto* t : tracks) {
            if (t->getName().containsIgnoreCase("Master")) return t;
        }
        return nullptr;
    }

    void removeTrack(int index) {
        std::unique_ptr<juce::ScopedLock> lock;
        if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

        if (index >= 0 && index < tracks.size()) {
            auto* t = tracks[index];
            for (int i = t->getAudioClips().size() - 1; i >= 0; --i) {
                auto* a = t->getAudioClips()[i];
                t->getAudioClips().removeObject(a, false);
                unusedAudioPool.add(a);
            }
            for (int i = t->getMidiClips().size() - 1; i >= 0; --i) {
                auto* m = t->getMidiClips()[i];
                t->getMidiClips().removeObject(m, false);
                unusedMidiPool.add(m);
            }
            trackPanels.remove(index);
            tracks.remove(index);
            if (index == lastSelectedTrackIndex) lastSelectedTrackIndex = -1;
            else if (index < lastSelectedTrackIndex) lastSelectedTrackIndex--;
            recalculateFolderHierarchy(); resized(); repaint();
        }
    }

    void deleteUnusedClipsByName(const juce::String& name, bool isMidi) {
        std::unique_ptr<juce::ScopedLock> lock;
        if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);
        if (isMidi) {
            for (int i = unusedMidiPool.size() - 1; i >= 0; --i) if (unusedMidiPool[i]->getName() == name) unusedMidiPool.remove(i, true);
        } else {
            for (int i = unusedAudioPool.size() - 1; i >= 0; --i) if (unusedAudioPool[i]->getName() == name) unusedAudioPool.remove(i, true);
        }
    }

    AutomationClipData* createAutomation(int targetTrackId, int parameterId, const juce::String& paramName) {
        std::unique_ptr<juce::ScopedLock> lock;
        if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

        for (auto* a : automationPool) {
            if (a->targetTrackId == targetTrackId && a->parameterId == parameterId) return a;
        }

        Track* target = nullptr;
        for (auto* t : tracks) { if (t->getId() == targetTrackId) { target = t; break; } }
        if (!target) return nullptr;

        auto* a = new AutomationClipData();
        a->name = target->getName() + " - " + paramName;
        a->targetTrackId = targetTrackId;
        a->parameterId = parameterId;
        a->color = target->getColor();
        a->lane.defaultValue = (parameterId == 0) ? 0.8f : 0.0f; // 0.8 is default volume, 0.0 is center pan
        
        automationPool.add(a);
        target->automationClips.push_back(a);
        target->commitSnapshot();
        if (onActiveTrackChanged) onActiveTrackChanged(target); // Force UI refresh
        return a;
    }

    void setVOffset(int newOffset) {
        vOffset = newOffset;
        resized();
    }

    void recalculateFolderHierarchy() {
        std::map<int, bool> folderCollapsedStates;
        for (auto* t : tracks) if (t->getType() == TrackType::Folder) folderCollapsedStates[t->getId()] = t->isCollapsed;

        for (int i = 0; i < tracks.size(); ++i) {
            auto* t = tracks[i];
            int depth = 0;
            int curr = t->parentId;
            int s1 = 0;
            while (curr != -1 && s1 < 32) {
                depth++;
                bool foundParent = false;
                for (auto* pt : tracks) if (pt->getId() == curr) { curr = pt->parentId; foundParent = true; break; }
                if (!foundParent) break;
                s1++;
            }
            t->folderDepth = depth;

            t->isShowingInChildren = true;
            int currParentId = t->parentId;
            int safety = 0;
            while (currParentId != -1 && safety < 32) {
                if (folderCollapsedStates.count(currParentId) && folderCollapsedStates[currParentId]) { t->isShowingInChildren = false; break; }
                bool found = false;
                for (auto* pt : tracks) if (pt->getId() == currParentId) { currParentId = pt->parentId; found = true; break; }
                if (!found) break;
                safety++;
            }

            auto* p = trackPanels[i];
            p->setVisible(t->isShowingInChildren);
            p->updateFolderBtnVisuals(); p->resized(); p->repaint();
        }
    }

    void moveSelectedTracksToNewFolder() {
        std::unique_ptr<juce::ScopedLock> lock;
        if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);
        std::vector<Track*> selected;
        int firstIdx = -1;
        for (int i = 0; i < tracks.size(); ++i) {
            if (tracks[i]->isSelected) { selected.push_back(tracks[i]); if (firstIdx == -1) firstIdx = i; }
        }
        if (selected.empty()) return;
        auto* folder = addTrack(TrackType::Folder);
        int fIdx = tracks.indexOf(folder);
        auto fPvt = tracks.removeAndReturn(fIdx); auto pPvt = trackPanels.removeAndReturn(fIdx);
        if (fIdx < firstIdx) firstIdx--;
        tracks.insert(firstIdx, fPvt); trackPanels.insert(firstIdx, pPvt);
        // IMPORTANTE: Asegurar que el panel de la carpeta también esté en trackContent
        trackContent.addAndMakeVisible(pPvt);
        for (auto* t : selected) t->parentId = folder->getId();
        recalculateFolderHierarchy(); resized(); repaint(); if (onTracksReordered) onTracksReordered();
    }

    void refreshTrackPanels() { for (auto* p : trackPanels) p->updatePlugins(); }

    void setAnalysisTrackToggleState(TrackType type, bool state) {
        if (type == TrackType::Loudness) toggleLdnBtn.setToggleState(state, juce::dontSendNotification);
        else if (type == TrackType::Balance) toggleBalBtn.setToggleState(state, juce::dontSendNotification);
        else if (type == TrackType::MidSide) toggleMsBtn.setToggleState(state, juce::dontSendNotification);
    }

    bool isInterestedInDragSource(const SourceDetails& d) override { return d.description == "TRACK"; }
    void itemDragMove(const SourceDetails& d) override {
        if (d.description != "TRACK") return;
        auto* drg = dynamic_cast<TrackControlPanel*>(d.sourceComponent.get());
        if (!drg) return;
        draggedPanelForGhost = drg;
        if (ghostSnapshot.isNull()) ghostSnapshot = drg->createComponentSnapshot(drg->getLocalBounds());
        
        // COMPENSACIÓN DE COORDENADAS: Usamos localPosition relativa a TrackContainer (que incluye la cabecera)
        ghostY = d.localPosition.y - (drg->getHeight() / 2);
        
        for (auto* p : trackPanels) {
            if (!p->isVisible()) continue;
            // Traducir localPosition al espacio de coordenadas de trackContent
            juce::Point<int> pRel = d.localPosition - trackContent.getPosition();
            if (p->getBounds().contains(pRel) && p != drg) {
                float nY = (float)(pRel.y - p->getY()) / (float)p->getHeight();
                if (p->track.getType() == TrackType::Folder && nY > 0.3f && nY < 0.7f) p->dragHoverMode = 2;
                else p->dragHoverMode = (nY < 0.5f) ? 1 : 3;
            } else { p->dragHoverMode = 0; }
            p->repaint();
        }
        repaint();
    }
    void itemDragExit(const SourceDetails& d) override {
        draggedPanelForGhost = nullptr; ghostSnapshot = juce::Image();
        for (auto* p : trackPanels) { p->dragHoverMode = 0; p->repaint(); }
        repaint();
    }
    void itemDropped(const SourceDetails& d) override {
        if (d.description != "TRACK") return;
        draggedPanelForGhost = nullptr; ghostSnapshot = juce::Image();
        auto* drg = dynamic_cast<TrackControlPanel*>(d.sourceComponent.get());
        if (!drg) return;
        int sIdx = tracks.indexOf(&drg->track);
        int tIdx = -1, mode = 0;
        
        juce::Point<int> pRel = d.localPosition - trackContent.getPosition();
        for (int i = 0; i < trackPanels.size(); ++i) {
            if (trackPanels[i]->isVisible() && trackPanels[i]->getBounds().contains(pRel)) {
                tIdx = i; float nY = (float)(pRel.y - trackPanels[i]->getY()) / (float)trackPanels[i]->getHeight();
                if (trackPanels[i]->track.getType() == TrackType::Folder && nY > 0.3f && nY < 0.7f) mode = 2;
                else mode = (nY < 0.5f) ? 1 : 3; break;
            }
        }
        for (auto* p : trackPanels) { p->dragHoverMode = 0; p->repaint(); }
        if (tIdx >= 0) {
            std::unique_ptr<juce::ScopedLock> lock; if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);
            if (mode == 2 && tracks[tIdx]->getType() == TrackType::Folder) {
                if (sIdx != tIdx) { tracks[sIdx]->parentId = tracks[tIdx]->getId(); }
            } else if (sIdx != tIdx) {
                tracks[sIdx]->parentId = -1;
                auto tM = tracks.removeAndReturn(sIdx); auto pM = trackPanels.removeAndReturn(sIdx);
                if (sIdx < tIdx) tIdx--;
                tracks.insert(mode == 1 ? tIdx : tIdx + 1, tM); trackPanels.insert(mode == 1 ? tIdx : tIdx + 1, pM);
            }
            recalculateFolderHierarchy(); resized(); repaint(); if (onTracksReordered) onTracksReordered();
        }
    }

    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void filesDropped(const juce::StringArray& files, int, int) override {
        std::unique_ptr<juce::ScopedLock> lock; if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);
        for (auto path : files) {
            juce::File f(path); if (f.getFileExtension().containsIgnoreCase("wav")) {
                addTrack(TrackType::Audio); tracks.getLast()->setName(f.getFileNameWithoutExtension());
            }
        }
        resized();
    }

    void paint(juce::Graphics& g) override { 
        if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
            g.fillAll(theme->getSkinColor("TRACKS_BG", juce::Colour(25, 27, 30)));
        } else {
            g.fillAll(juce::Colour(25, 27, 30));
        }
    }
    void paintOverChildren(juce::Graphics& g) override {
        if (draggedPanelForGhost != nullptr && !ghostSnapshot.isNull()) {
            g.setOpacity(0.6f); g.drawImageAt(ghostSnapshot, 0, ghostY);
        }
    }

    void resized() override {
        auto area = getLocalBounds();
        auto top = area.removeFromTop(120); // GRID FIX: Cabecera 120px
        headerBg.setBounds(top);
        
        // --- CLIPPING: El área de contenido empieza después de la cabecera ---
        trackContent.setBounds(area);
        
        int currentY = -vOffset; // Relativo a trackContent
        for (auto* p : trackPanels) {
            if (p->isVisible()) {
                p->setBounds(0, currentY, trackContent.getWidth(), (int)trackHeight); 
                currentY += (int)trackHeight;
            }
        }
        addMidiBtn.setBounds(5, 5, 55, 25); 
        addAudioBtn.setBounds(65, 5, 55, 25);
        
        toggleLdnBtn.setBounds(125, 5, 38, 25);
        toggleBalBtn.setBounds(167, 5, 38, 25);
        toggleMsBtn.setBounds(209, 5, 36, 25);

        headerBg.toBack(); 
        addMidiBtn.toFront(false); addAudioBtn.toFront(false);
        toggleLdnBtn.toFront(false); toggleBalBtn.toFront(false); toggleMsBtn.toFront(false);
    }

    void updateStyles() {
        if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
            auto bg = theme->getSkinColor("TRACKS_BG", juce::Colour(25, 27, 30));
            addMidiBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
            addAudioBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
            toggleLdnBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
            toggleBalBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
            toggleMsBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
            
            // Colores activos para los "Switches"
            toggleLdnBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange.withAlpha(0.6f));
            toggleBalBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::cyan.withAlpha(0.6f));
            toggleMsBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::magenta.withAlpha(0.6f));
        }
    }

    void lookAndFeelChanged() override {
        updateStyles();
        headerBg.repaint();
        repaint();
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override {
        if (onScrollWheel) onScrollWheel(wheel.deltaY); juce::Component::mouseWheelMove(e, wheel);
    }

private:
    TrackHeaderBackground headerBg;
    juce::TextButton addMidiBtn{ "+ MIDI" }, addAudioBtn{ "+ AUDIO" };
    juce::TextButton toggleLdnBtn{ "LDN" }, toggleBalBtn{ "BAL" }, toggleMsBtn{ "MS" };
    juce::OwnedArray<Track> tracks;
    juce::OwnedArray<TrackControlPanel> trackPanels;
    juce::CriticalSection* audioMutex = nullptr;
    TrackControlPanel* draggedPanelForGhost = nullptr;
    int ghostY = 0; juce::Image ghostSnapshot;
    int lastSelectedTrackIndex = -1;
    int nextTrackId = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackContainer)
};