#include "TrackContainer.h"

TrackContainer::TrackContainer() 
{
    addAndMakeVisible(trackContent);
    addAndMakeVisible(headerBg);
    headerBg.setInterceptsMouseClicks(true, false);

    addAndMakeVisible(addMidiBtn);
    addAndMakeVisible(addAudioBtn);

    addMidiBtn.onClick = [this] { addTrack(TrackType::MIDI); };
    addAudioBtn.onClick = [this] { addTrack(TrackType::Audio); };

    addAndMakeVisible(toggleLdnBtn);
    addAndMakeVisible(toggleBalBtn);
    addAndMakeVisible(toggleMsBtn);

    toggleLdnBtn.setClickingTogglesState(true);
    toggleBalBtn.setClickingTogglesState(true);
    toggleMsBtn.setClickingTogglesState(true);

    toggleLdnBtn.onClick = [this] { if (onToggleAnalysisTrack) onToggleAnalysisTrack(TrackType::Loudness, toggleLdnBtn.getToggleState()); };
    toggleBalBtn.onClick = [this] { if (onToggleAnalysisTrack) onToggleAnalysisTrack(TrackType::Balance, toggleBalBtn.getToggleState()); };
    toggleMsBtn.onClick = [this] { if (onToggleAnalysisTrack) onToggleAnalysisTrack(TrackType::MidSide, toggleMsBtn.getToggleState()); };
    
    startTimerHz(15); // Activar sincronización entre ventanas
}

void TrackContainer::setTrackHeight(float newHeight) 
{
    trackHeight = newHeight;
    resized();
    repaint();
}

void TrackContainer::deselectAllTracks() 
{
    for (auto* t : getTracks()) t->isSelected = false;
    for (auto* p : trackPanels) p->repaint();
}

void TrackContainer::selectTrack(Track* selectedTrack, const juce::ModifierKeys& mods) 
{
    int clickedIndex = getTracks().indexOf(selectedTrack);
    if (clickedIndex < 0) return;

    if (mods.isCommandDown() || mods.isCtrlDown()) {
        selectedTrack->isSelected = !selectedTrack->isSelected;
        lastSelectedTrackIndex = clickedIndex;
        if (selectedTrack->isSelected && onDeselectMasterRequested) onDeselectMasterRequested();
    }
    else if (mods.isShiftDown()) {
        if (lastSelectedTrackIndex >= 0 && lastSelectedTrackIndex < getTracks().size()) {
            int start = std::min(lastSelectedTrackIndex, clickedIndex);
            int end = std::max(lastSelectedTrackIndex, clickedIndex);
            for (int i = start; i <= end; ++i) getTracks()[i]->isSelected = true;
        }
        else {
            selectedTrack->isSelected = true;
            lastSelectedTrackIndex = clickedIndex;
        }
        if (onDeselectMasterRequested) onDeselectMasterRequested();
    }
    else {
        for (auto* t : getTracks()) t->isSelected = false;
        selectedTrack->isSelected = true;
        lastSelectedTrackIndex = clickedIndex;
        if (onDeselectMasterRequested) onDeselectMasterRequested();
    }

    for (auto* p : trackPanels) p->repaint();
    if (onActiveTrackChanged) onActiveTrackChanged(selectedTrack);
}

Track* TrackContainer::addTrack(TrackType type, int idToUse) 
{
    std::unique_ptr<juce::ScopedLock> lock;
    if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

    int id = idToUse > 0 ? idToUse : nextTrackId++;
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
    getTracksMutable().add(t);
    auto* p = new TrackControlPanel(*t);

    p->onFxClick = [this, t, p] { if (onLoadFx) onLoadFx(*t, *p); };
    p->onInstrumentClick = [this, t] { if (onOpenInstrument) onOpenInstrument(*t); };
    p->onPluginClick = [this, t](int i) { if (onOpenFx) onOpenFx(*t, i); };
    p->onPianoRollClick = [this, t] { if (onOpenPianoRoll) onOpenPianoRoll(*t); };
    p->onDeleteClick = [this, t] {
        juce::MessageManager::callAsync([this, t] {
            if (!getTracks().contains(t)) return;
            if (t->isSelected) {
                std::vector<int> toDelete;
                for (int i = 0; i < getTracks().size(); ++i) if (getTracks()[i]->isSelected) toDelete.push_back(i);
                for (int i = (int)toDelete.size() - 1; i >= 0; --i) if (onDeleteTrack) onDeleteTrack(toDelete[i]);
            } else {
                if (onDeleteTrack) onDeleteTrack(getTracks().indexOf(t));
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
            for (auto* trk : getTracks()) if (trk->isSelected) trk->setColor(nc);
        }
        repaint(); if (getParentComponent()) getParentComponent()->repaint();
    };

    p->onCreateAutomation = [this, t](int paramId, juce::String paramName) {
        createAutomation(t->getId(), paramId, paramName);
    };

    trackPanels.add(p);
    trackContent.addAndMakeVisible(p);
    recalculateFolderHierarchy();
    resized();
    if (onTracksReordered) onTracksReordered();
    if (onTrackAdded) onTrackAdded();
    return t;
}

Track* TrackContainer::getMasterTrack() 
{
    for (auto* t : getTracks()) {
        if (t->getName().containsIgnoreCase("Master")) return t;
    }
    return nullptr;
}

void TrackContainer::removeTrack(int index) 
{
    std::unique_ptr<juce::ScopedLock> lock;
    if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

    if (index >= 0 && index < getTracks().size()) {
        auto* t = getTracks()[index];
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
        getTracksMutable().remove(index);
        if (index == lastSelectedTrackIndex) lastSelectedTrackIndex = -1;
        else if (index < lastSelectedTrackIndex) lastSelectedTrackIndex--;
        recalculateFolderHierarchy(); resized(); repaint();
    }
}

void TrackContainer::deleteUnusedClipsByName(const juce::String& name, bool isMidi) 
{
    std::unique_ptr<juce::ScopedLock> lock;
    if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);
    if (isMidi) {
        for (int i = unusedMidiPool.size() - 1; i >= 0; --i) if (unusedMidiPool[i]->getName() == name) unusedMidiPool.remove(i, true);
    } else {
        for (int i = unusedAudioPool.size() - 1; i >= 0; --i) if (unusedAudioPool[i]->getName() == name) unusedAudioPool.remove(i, true);
    }
}

AutomationClipData* TrackContainer::createAutomation(int targetTrackId, int parameterId, const juce::String& paramName) 
{
    std::unique_ptr<juce::ScopedLock> lock;
    if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

    for (auto* a : automationPool) {
        if (a->targetTrackId == targetTrackId && a->parameterId == parameterId) return a;
    }

    Track* target = nullptr;
    for (auto* t : getTracks()) { if (t->getId() == targetTrackId) { target = t; break; } }
    if (!target) return nullptr;

    auto* a = new AutomationClipData();
    a->name = target->getName() + " - " + paramName;
    a->targetTrackId = targetTrackId;
    a->parameterId = parameterId;
    a->color = target->getColor();
    a->lane.defaultValue = (parameterId == 0) ? 0.8f : 0.0f;
    
    automationPool.add(a);
    target->automationClips.push_back(a);
    target->commitSnapshot();
    if (onActiveTrackChanged) onActiveTrackChanged(target);
    return a;
}

void TrackContainer::recalculateFolderHierarchy() 
{
    std::map<int, bool> folderCollapsedStates;
    for (auto* t : getTracks()) if (t->getType() == TrackType::Folder) folderCollapsedStates[t->getId()] = t->isCollapsed;

    for (int i = 0; i < getTracks().size(); ++i) {
        auto* t = getTracks()[i];
        int depth = 0;
        int curr = t->parentId;
        int s1 = 0;
        while (curr != -1 && s1 < 32) {
            depth++;
            bool foundParent = false;
            for (auto* pt : getTracks()) if (pt->getId() == curr) { curr = pt->parentId; foundParent = true; break; }
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
            for (auto* pt : getTracks()) if (pt->getId() == currParentId) { currParentId = pt->parentId; found = true; break; }
            if (!found) break;
            safety++;
        }

        if (i < trackPanels.size()) {
            auto* p = trackPanels[i];
            p->setVisible(t->isShowingInChildren);
            p->updateFolderBtnVisuals(); p->resized(); p->repaint();
        }
    }
}

void TrackContainer::moveSelectedTracksToNewFolder() 
{
    std::unique_ptr<juce::ScopedLock> lock;
    if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);
    std::vector<Track*> selected;
    int firstIdx = -1;
    for (int i = 0; i < getTracks().size(); ++i) {
        if (getTracks()[i]->isSelected) { selected.push_back(getTracks()[i]); if (firstIdx == -1) firstIdx = i; }
    }
    if (selected.empty()) return;
    auto* folder = addTrack(TrackType::Folder);
    int fIdx = getTracks().indexOf(folder);
    auto fPvt = getTracksMutable().removeAndReturn(fIdx); auto pPvt = trackPanels.removeAndReturn(fIdx);
    if (fIdx < firstIdx) firstIdx--;
    getTracksMutable().insert(firstIdx, fPvt); trackPanels.insert(firstIdx, pPvt);
    trackContent.addAndMakeVisible(pPvt);
    for (auto* t : selected) t->parentId = folder->getId();
    recalculateFolderHierarchy(); resized(); repaint(); if (onTracksReordered) onTracksReordered();
}

void TrackContainer::itemDragMove(const SourceDetails& d) 
{
    if (d.description != "TRACK") return;
    auto* drg = dynamic_cast<TrackControlPanel*>(d.sourceComponent.get());
    if (!drg) return;
    
    // (Ghost image logic removed)
    
    for (auto* p : trackPanels) {
        if (!p->isVisible()) continue;
        juce::Point<int> pRel = d.localPosition - trackContent.getPosition();
        if (p->getBounds().contains(pRel) && p != drg) {
            float nY = (float)(pRel.y - p->getY()) / (float)p->getHeight();
            
            // Permitimos "soltar dentro" (mode 2) si es un Folder O si es una pista normal y está vacía
            bool canBeFolder = p->track.getType() == TrackType::Folder || 
                               (p->track.isEmpty() && (p->track.getType() == TrackType::Audio || p->track.getType() == TrackType::MIDI));

            if (canBeFolder && nY > 0.3f && nY < 0.7f) p->dragHoverMode = 2;
            else p->dragHoverMode = (nY < 0.5f) ? 1 : 3;
        } else { p->dragHoverMode = 0; }
        p->repaint();
    }
    repaint();
}

void TrackContainer::itemDropped(const SourceDetails& d) 
{
    if (d.description != "TRACK") return;
    auto* drg = dynamic_cast<TrackControlPanel*>(d.sourceComponent.get());
    if (!drg) return;
    int sIdx = getTracks().indexOf(&drg->track);
    int tIdx = -1, mode = 0;
    
    juce::Point<int> pRel = d.localPosition - trackContent.getPosition();
    for (int i = 0; i < trackPanels.size(); ++i) {
        // SEGURIDAD: No podemos soltar una pista sobre sí misma (evita el "rebote" de conversión)
        if (trackPanels[i]->isVisible() && trackPanels[i]->getBounds().contains(pRel) && trackPanels[i] != drg) {
            tIdx = i; float nY = (float)(pRel.y - trackPanels[i]->getY()) / (float)trackPanels[i]->getHeight();
            bool canBeFolder = trackPanels[i]->track.getType() == TrackType::Folder || 
                               (trackPanels[i]->track.isEmpty() && (trackPanels[i]->track.getType() == TrackType::Audio || trackPanels[i]->track.getType() == TrackType::MIDI));

            if (canBeFolder && nY > 0.3f && nY < 0.7f) mode = 2;
            else mode = (nY < 0.5f) ? 1 : 3; break;
        }
    }
    for (auto* p : trackPanels) { p->dragHoverMode = 0; p->repaint(); }
    if (tIdx >= 0) {
        std::unique_ptr<juce::ScopedLock> lock; if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);
        
        Track* draggedTrack = &drg->track;
        Track* targetTrack = getTracks()[tIdx];

        // 1. Identificar TODAS las pistas a mover (el padre + todos sus hijos recursivos)
        std::vector<Track*> children = getRecursiveChildren(draggedTrack);
        std::vector<int> indicesToMove;
        indicesToMove.push_back(getTracks().indexOf(draggedTrack));
        for (auto* c : children) {
            int idx = getTracks().indexOf(c);
            if (idx >= 0) indicesToMove.push_back(idx);
        }
        std::sort(indicesToMove.begin(), indicesToMove.end());

        // 2. Si es modo 2 (soltar dentro), gestionar la conversión a carpeta y nueva paternidad
        if (mode == 2) {
            if (targetTrack->getType() != TrackType::Folder && targetTrack->isEmpty()) {
                targetTrack->setType(TrackType::Folder);
            }
            if (targetTrack->getType() == TrackType::Folder && draggedTrack != targetTrack) {
                draggedTrack->parentId = targetTrack->getId();
            }
        } else {
            // Herencia inteligente: al reordenar, heredamos el nivel de jerarquía (padre) del objetivo
            if (draggedTrack != targetTrack) {
                draggedTrack->parentId = targetTrack->parentId;
            }
        }

        // 3. Extraer el bloque completo de pistas y paneles
        std::vector<Track*> tracksToMove;
        std::vector<TrackControlPanel*> panelsToMove;
        
        // Removemos de mayor a menor índice para no alterar los índices de las que faltan por quitar
        std::vector<int> sortedIndices = indicesToMove;
        std::sort(sortedIndices.rbegin(), sortedIndices.rend());
        
        for (int idx : sortedIndices) {
            tracksToMove.insert(tracksToMove.begin(), getTracksMutable().removeAndReturn(idx));
            panelsToMove.insert(panelsToMove.begin(), trackPanels.removeAndReturn(idx));
        }

        // 4. Calcular el nuevo índice de destino tras haber quitado las pistas
        int finalTargetIdx = getTracks().indexOf(targetTrack);
        if (mode != 1) finalTargetIdx++; // Para modo 2 y 3 insertamos después del target

        // 5. Insertar todo el bloque en la nueva posición
        for (int i = 0; i < tracksToMove.size(); ++i) {
            getTracksMutable().insert(finalTargetIdx + i, tracksToMove[i]);
            trackPanels.insert(finalTargetIdx + i, panelsToMove[i]);
        }

        recalculateFolderHierarchy(); resized(); repaint(); if (onTracksReordered) onTracksReordered();
    }
}

void TrackContainer::filesDropped(const juce::StringArray& files, int, int) 
{
    std::unique_ptr<juce::ScopedLock> lock; if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);
    for (auto path : files) {
        juce::File f(path); if (f.getFileExtension().containsIgnoreCase("wav")) {
            addTrack(TrackType::Audio); tracks.getLast()->setName(f.getFileNameWithoutExtension());
        }
    }
    resized();
}

void TrackContainer::paint(juce::Graphics& g) 
{ 
    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
        g.fillAll(theme->getSkinColor("TRACKS_BG", juce::Colour(25, 27, 30)));
    } else {
        g.fillAll(juce::Colour(25, 27, 30));
    }
}

void TrackContainer::resized() 
{
    auto area = getLocalBounds();
    auto top = area.removeFromTop(120);
    headerBg.setBounds(top);
    trackContent.setBounds(area);
    
    int currentY = -vOffset;
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

void TrackContainer::updateStyles() 
{
    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
        auto bg = theme->getSkinColor("TRACKS_BG", juce::Colour(25, 27, 30));
        addMidiBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
        addAudioBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
        toggleLdnBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
        toggleBalBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
        toggleMsBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
        
        toggleLdnBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange.withAlpha(0.6f));
        toggleBalBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::cyan.withAlpha(0.6f));
        toggleMsBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::magenta.withAlpha(0.6f));
    }
}

void TrackContainer::refreshTrackPanels() 
{ 
    // Sincronizamos paneles si hay un desajuste (aplica tanto a maestro como a clon)
    if (getTracks().size() != trackPanels.size())
    {
        trackPanels.clear();
        for (auto* t : getTracks())
        {
            auto* p = new TrackControlPanel(*t);
            
            p->onFxClick = [this, t, p] { if (onLoadFx) onLoadFx(*t, *p); };
            p->onInstrumentClick = [this, t] { if (onOpenInstrument) onOpenInstrument(*t); };
            p->onPluginClick = [this, t](int i) { if (onOpenFx) onOpenFx(*t, i); };
            p->onPianoRollClick = [this, t] { if (onOpenPianoRoll) onOpenPianoRoll(*t); };
            
            p->onDeleteClick = [this, t] {
                juce::MessageManager::callAsync([this, t] {
                    if (!getTracks().contains(t)) return;
                    if (t->isSelected) {
                        std::vector<int> toDelete;
                        for (int i = 0; i < getTracks().size(); ++i) if (getTracks()[i]->isSelected) toDelete.push_back(i);
                        for (int i = (int)toDelete.size() - 1; i >= 0; --i) if (onDeleteTrack) onDeleteTrack(toDelete[i]);
                    } else {
                        if (onDeleteTrack) onDeleteTrack(getTracks().indexOf(t));
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
                    for (auto* trk : getTracks()) if (trk->isSelected) trk->setColor(nc);
                }
                repaint(); if (getParentComponent()) getParentComponent()->repaint();
            };

            p->onCreateAutomation = [this, t](int paramId, juce::String paramName) {
                createAutomation(t->getId(), paramId, paramName);
            };
            
            trackPanels.add(p);
            trackContent.addAndMakeVisible(p);
        }
        
        recalculateFolderHierarchy();
        resized();
    }

    for (auto* p : trackPanels) p->updatePlugins(); 
}

void TrackContainer::setAnalysisTrackToggleState(TrackType type, bool state) 
{
    if (type == TrackType::Loudness) toggleLdnBtn.setToggleState(state, juce::dontSendNotification);
    else if (type == TrackType::Balance) toggleBalBtn.setToggleState(state, juce::dontSendNotification);
    else if (type == TrackType::MidSide) toggleMsBtn.setToggleState(state, juce::dontSendNotification);
}

void TrackContainer::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) 
{
    if (onScrollWheel) onScrollWheel(wheel.deltaY); 
    juce::Component::mouseWheelMove(e, wheel);
}

void TrackContainer::copyCallbacksFrom(const TrackContainer& other)
{
    onLoadFx = other.onLoadFx;
    onOpenInstrument = other.onOpenInstrument;
    onOpenFx = other.onOpenFx;
    onOpenPianoRoll = other.onOpenPianoRoll;
    onDeleteTrack = other.onDeleteTrack;
    onOpenEffects = other.onOpenEffects;
    onTracksReordered = other.onTracksReordered;
    onTrackAdded = other.onTrackAdded;
    onActiveTrackChanged = other.onActiveTrackChanged;
    onToggleAnalysisTrack = other.onToggleAnalysisTrack;
    onDeselectMasterRequested = other.onDeselectMasterRequested;
    
    // Forzar actualización de los paneles con los nuevos callbacks
    refreshTrackPanels();
}

std::vector<Track*> TrackContainer::getRecursiveChildren(Track* parent)
{
    std::vector<Track*> result;
    if (!parent) return result;
    
    int parentId = parent->getId();
    for (auto* t : getTracks()) {
        if (t->parentId == parentId) {
            result.push_back(t);
            auto children = getRecursiveChildren(t);
            result.insert(result.end(), children.begin(), children.end());
        }
    }
    return result;
}
