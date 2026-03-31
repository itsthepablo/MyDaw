#include "MainComponent.h"

MainComponent::MainComponent() {
    setupCommands();
    commandManager.registerAllCommandsForTarget(this);
    addKeyListener(commandManager.getKeyMappings());

    ui.resourceMeter = std::make_unique<ResourceMeter>(*this);

    UIManager::setupUI(ui, *this, [this] { closePianoRoll(); });
    ui.onResized = [this] { resized(); };

    setupCallbacks();
    setupBridges();

    ui.trackContainer.setExternalMutex(&audioMutex);
    ui.playlistUI.setExternalMutex(&audioMutex);
    ui.playlistUI.setTrackContainer(&ui.trackContainer);
    ui.pickerPanelUI.setTrackContainer(&ui.trackContainer);

    setSize(1920, 1080);

    setAudioChannels(0, 2);
}

MainComponent::~MainComponent() { shutdownAudio(); }

void MainComponent::setupCommands() {
    commandHandler = std::make_unique<DAWCommandHandler>(CommandActions{
        [this] { ui.topMenuBar.playBtn.triggerClick(); },
        [this] { toggleViewMode(); }
        });
}

void MainComponent::setupCallbacks() {
    ui.topMenuBar.viewToggleBtn.onClick = [this] { toggleViewMode(); };
    ui.topMenuBar.onSaveRequested = [this] { saveProject(); };
    ui.topMenuBar.onExportRequested = [this] { startExport(); };

    // --- CONEXIÓN METRÓNOMO (UI -> DSP) ---
    ui.topMenuBar.metronomeBtn.onClick = [this] {
        audioEngine.metronome.setEnabled(ui.topMenuBar.metronomeBtn.getToggleState());
        };

    ui.playlistUI.onNewTrackRequested = [this](TrackType type) { ui.trackContainer.addTrack(type); };

    ui.playlistUI.onVerticalScroll = [this](int scrollPos) {
        ui.trackContainer.setVOffset(scrollPos);
        };

    ui.trackContainer.onScrollWheel = [this](float deltaY) {
        double currentStart = ui.playlistUI.vBar.getCurrentRangeStart();
        double currentSize = ui.playlistUI.vBar.getCurrentRangeSize();
        double maxLimit = ui.playlistUI.vBar.getMaximumRangeLimit();
        double newStart = juce::jlimit(0.0, std::max(0.0, maxLimit - currentSize), currentStart - (deltaY * 100.0));
        ui.playlistUI.vBar.setCurrentRange(newStart, currentSize);
        ui.playlistUI.updateScrollBars();
        ui.playlistUI.repaint();
        };

    ui.playlistUI.onClipSelected = [this](Track* t) {
        ui.trackContainer.selectTrack(t, juce::ModifierKeys());
        ui.effectsPanelUI.setTrack(t);
        ui.instrumentPanelUI.setTrack(t);
        };

    // --- CONEXIÓN MASTER TRACK ---
    ui.masterStrip.onTrackSelected = [this](Track* mt) {
        ui.trackContainer.deselectAllTracks();
        ui.bottomDock.showTab(BottomDock::EffectsTab);
        ui.effectsPanelUI.setTrack(mt);
        isBottomDockVisible = true;
        resized();
    };

    ui.trackContainer.onOpenEffects = [this](Track& track) {
        ui.masterStrip.setSelected(false);
        ui.bottomDock.showTab(BottomDock::EffectsTab);
        ui.effectsPanelUI.setTrack(&track);
        isBottomDockVisible = true;
        resized();
    };

    ui.trackContainer.onTrackAdded = [this] {
        auto* newTrack = ui.trackContainer.getTracks().getLast();
        if (newTrack) {
            // MAX SIZE estricto para evitar bounds overwrite failures
            int maxSamples = 8192;
            newTrack->audioBuffer.setSize(2, maxSamples, false, true, false);
            newTrack->instrumentMixBuffer.setSize(2, maxSamples, false, true, false);
            newTrack->tempSynthBuffer.setSize(2, maxSamples, false, true, false);
            newTrack->midSideBuffer.setSize(1, maxSamples, false, true, false);
            // pdcBuffer NO se pre-aloca para pistas vacias (ahorra 4MB por pista).
            // Se aloca bajo demanda en allocatePdcBuffer() cuando se carga un plugin.
            // PDCManager::applyDelay() ya tiene guard 'if (bufferSize == 0) return'.
            // DOUBLE BUFFER: inicializar snapshot vacio antes de exponer el track al audio thread
            newTrack->commitSnapshot();
        }
        audioEngine.routingMatrix.commitNewTopology(ui.trackContainer.getTracks());
        };

    ui.trackContainer.onTracksReordered = [this] {
        audioEngine.routingMatrix.commitNewTopology(ui.trackContainer.getTracks());
        };


    ui.pickerPanelUI.onDeleteRequested = [this](juce::String name, bool isMidi) {
        ui.playlistUI.deleteClipsByName(name, isMidi);
        ui.trackContainer.deleteUnusedClipsByName(name, isMidi);
        ui.pickerPanelUI.refreshList();
        };

    ui.leftSidebar.onClose = [this] { isLeftSidebarVisible = false; resized(); };
    ui.bottomDock.onClose = [this] { isBottomDockVisible = false; resized(); };

    ui.sidebarResizer.onResizeDelta = [this](int d) {
        leftSidebarWidth = juce::jlimit(150, 600, leftSidebarWidth + d);
        resized();
        };

    ui.bottomDockResizer.onResizeDelta = [this](int d) {
        bottomDockHeight = juce::jlimit(100, (int)(getHeight() * 0.8f), bottomDockHeight - d);
        resized();
        };

    ui.playlistUI.onMidiClipDeleted = [this](MidiClipData* c) {
        if (ui.pianoRollUI.getActiveClip() == c) closePianoRoll();
        };

    ui.trackContainer.onDeleteTrack = [this](int index) {
        const juce::ScopedLock sl(audioMutex);
        if (index >= 0 && index < ui.trackContainer.getTracks().size()) {
            Track* trackToDelete = ui.trackContainer.getTracks()[index];
            BridgeManager::cleanupTrack(trackToDelete, ui.pianoRollUI, [this] { closePianoRoll(); });

            ui.playlistUI.purgeClipsOfTrack(trackToDelete);
            ui.trackContainer.removeTrack(index); 
            audioEngine.routingMatrix.commitNewTopology(ui.trackContainer.getTracks());

            ui.playlistUI.updateScrollBars();
            ui.playlistUI.repaint();
            ui.pickerPanelUI.refreshList();

            if (ui.bottomDock.getCurrentTab() == BottomDock::EffectsTab)
                ui.effectsPanelUI.setTrack(nullptr);

            if (ui.bottomDock.getCurrentTab() == BottomDock::InstrumentTab)
                ui.instrumentPanelUI.setTrack(nullptr);

            resized();
        }
    };

    // --- CONEXIÓN SIDECHAIN (DATA SOURCE) ---
    ui.effectsPanelUI.getAvailableTracks = [this]() -> juce::Array<Track*> {
        juce::Array<Track*> trackList;
        for (auto* t : ui.trackContainer.getTracks()) 
            trackList.add(t);
        return trackList;
    };
}

void MainComponent::setupBridges() {
    BridgeManager::initializeAllBridges({
        ui.trackContainer, ui.playlistUI, ui.pianoRollUI, ui.mixerUI, ui.miniMixerUI, ui.effectsPanelUI,
        ui.instrumentPanelUI, 
        ui.transportBar, ui.topMenuBar, ui.bottomDock, ui.leftSidebar, audioEngine, audioMutex,
        isBottomDockVisible, isLeftSidebarVisible,
        [this] { openPianoRoll(); }, [this] { closePianoRoll(); },
        [this] { resized(); }, [this] { toggleViewMode(); },
        [this] {
            if (isPianoRollVisible) closePianoRoll();
            currentView = ViewMode::Arrangement;
            isBottomDockVisible = true;
            ui.bottomDock.showTab(BottomDock::EffectsTab);
            resized();
        },
        ui.masterTrackObj.get()
        });
}

void MainComponent::prepareToPlay(int samples, double s) {
    audioEngine.prepareToPlay(samples, s);

    // Alocar recursos estables para pistas cargadas previamente via Proyecto
    for (auto* t : ui.trackContainer.getTracks()) {
        int maxSamples = 8192;
        t->audioBuffer.setSize(2, maxSamples, false, true, false);
        t->instrumentMixBuffer.setSize(2, maxSamples, false, true, false);
        t->tempSynthBuffer.setSize(2, maxSamples, false, true, false);
        t->midSideBuffer.setSize(1, maxSamples, false, true, false);
        t->audioBuffer.clear();
        // pdcBuffer: solo alocar si el track tiene plugins (ahorra 4MB por pista vacia).
        // Si ya tenia tamano por carga previa de plugins, lo respetamos y limpiamos.
        bool hasPlugins = !t->plugins.isEmpty();
        if (hasPlugins) {
            t->allocatePdcBuffer();
            t->pdcBuffer.clear();
        }
        // DOUBLE BUFFER: publicar snapshot inicial con los clips/notas cargados del proyecto
        t->commitSnapshot();
    }

    // PREPARAR MASTER TRACK
    if (ui.masterTrackObj) {
        int maxSamples = 8192;
        auto* mt = ui.masterTrackObj.get();
        mt->audioBuffer.setSize(2, maxSamples, false, true, false);
        mt->instrumentMixBuffer.setSize(2, maxSamples, false, true, false);
        mt->tempSynthBuffer.setSize(2, maxSamples, false, true, false);
        mt->midSideBuffer.setSize(1, maxSamples, false, true, false);
        mt->audioBuffer.clear();
        mt->commitSnapshot();
        audioEngine.setMasterTrack(mt);
    }

    // Primera carga topologica
    audioEngine.routingMatrix.commitNewTopology(ui.trackContainer.getTracks());
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& buffer) {
    if (isOfflineRendering.load(std::memory_order_relaxed)) {
        buffer.clearActiveBufferRegion();
        return;
    }

    // Sincronización State UI -> Audio (Lock-Free)
    audioEngine.transportState.isPlaying.store(ui.pianoRollUI.getIsPlaying(), std::memory_order_relaxed);
    audioEngine.transportState.isPreviewing.store(ui.pianoRollUI.getIsPreviewing(), std::memory_order_relaxed);
    audioEngine.transportState.previewPitch.store(ui.pianoRollUI.getPreviewPitch(), std::memory_order_relaxed);
    audioEngine.transportState.bpm.store(ui.pianoRollUI.getBpm(), std::memory_order_relaxed);
    audioEngine.transportState.loopEndPos.store(juce::jmax(ui.pianoRollUI.getLoopEndPos(), ui.playlistUI.getLoopEndPos()), std::memory_order_relaxed);
    audioEngine.masterVolume.store(ui.mixerUI.getMasterVolume(), std::memory_order_relaxed);

    // PROTECCIÓN ANTI-CRASH: El audioMutex se toma brevemente para garantizar que
    // el audio thread no acceda a Track* que el UI thread acaba de destruir.
    // El ScopedTryLock evita bloquear: si no puede adquirirlo, produce silencio
    // en vez de crashear. El UI thread solo lo toma durante addTrack/removeTrack.
    {
        const juce::ScopedTryLock stl(audioMutex);
        if (stl.isLocked()) {
            audioEngine.processBlock(buffer);
        }
        else {
            // La UI está en medio de una operacion estructural (añadir/eliminar pistas).
            // Producir silencio este bloque es preferible a un crash por use-after-free.
            buffer.clearActiveBufferRegion();
        }
    }
}

void MainComponent::releaseResources() { audioEngine.releaseResources(); }

void MainComponent::paint(juce::Graphics& g) { g.fillAll(juce::Colour(30, 32, 35)); }

void MainComponent::toggleViewMode() {
    currentView = (currentView == ViewMode::Arrangement) ? ViewMode::Mixer : ViewMode::Arrangement;
    resized();
}

void MainComponent::openPianoRoll() {
    if (!isPianoRollVisible) {
        prePianoRollLeftSidebar = isLeftSidebarVisible;
        prePianoRollBottomDock = isBottomDockVisible;
        isPianoRollVisible = true;
        currentView = ViewMode::Arrangement;
        resized();
    }
}

void MainComponent::closePianoRoll() {
    if (isPianoRollVisible) {
        isPianoRollVisible = false;
        isLeftSidebarVisible = prePianoRollLeftSidebar;
        isBottomDockVisible = prePianoRollBottomDock;
        ui.pianoRollUI.setActiveClip(nullptr);
        resized();
    }
}

void MainComponent::loadProject(const juce::File& file) {
    ProjectManager::loadProject(file, ui.trackContainer, audioEngine, &audioMutex, ui.playlistUI, ui.effectsPanelUI, ui.pickerPanelUI, [this] { resized(); });
}

void MainComponent::saveProject() {
    ProjectManager::saveProject(ui.trackContainer, audioEngine, fileChooser);
}

// --- INYECCIÓN 3: LOGICA DE EXPORTACIÓN (ESTILO REAPER) ---
void MainComponent::startExport() {
    if (isOfflineRendering.load()) return;

    if (audioEngine.transportState.isPlaying.load()) {
        ui.topMenuBar.stopBtn.triggerClick();
    }

    if (!offlineRenderer) {
        offlineRenderer = std::make_unique<OfflineRenderer>();
        addChildComponent(offlineRenderer.get());

        offlineRenderer->onProcessOfflineBlock = [this](juce::AudioBuffer<float>& buffer, int numSamples) {
            juce::AudioSourceChannelInfo info(&buffer, 0, numSamples);
            audioEngine.processBlock(info);
        };

        offlineRenderer->onClose = [this] {
            audioEngine.transportState.isPlaying.store(false);
            
            // RESTAURAR ESTADO REAL-TIME
            audioEngine.setNonRealtime(false);
            audioEngine.prepareToPlay(512, deviceManager.getAudioDeviceSetup().sampleRate);
            audioEngine.resetForRender();

            offlineRenderer->setVisible(false);
            isOfflineRendering.store(false);
            };
    }

    // PREPARAR MOTOR PARA OFFLINE
    isOfflineRendering.store(true);
    audioEngine.setNonRealtime(true);
    audioEngine.resetForRender();
    audioEngine.transportState.isPlaying.store(true);

    double currentBpm = ui.playlistUI.getBpm();
    double timelinePixels = ui.playlistUI.getTimelineLength();
    double lengthSecs = (timelinePixels / 320.0) * 4.0 * (60.0 / currentBpm);

    offlineRenderer->setBounds(getLocalBounds().withSizeKeepingCentre(700, 500));
    offlineRenderer->setVisible(true);
    offlineRenderer->toFront(true);

    offlineRenderer->showConfig(lengthSecs);
}
// ------------------------------------------------------------

void MainComponent::resized() {
    LayoutHandler::performLayout({
        getLocalBounds(), ui.topMenuBar, ui.hintPanel, ui.resourceMeter.get(), ui.transportBar,
        ui.pianoRollUI, ui.closePianoRollBtn, ui.mixerUI, ui.masterChannelUI, ui.trackContainer, ui.playlistUI,
        ui.bottomDock, ui.bottomDockResizer, ui.leftSidebar, ui.sidebarResizer, ui.masterStrip,
        currentView, isPianoRollVisible, isBottomDockVisible, bottomDockHeight, isLeftSidebarVisible, leftSidebarWidth
        });
}

juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget() { return commandHandler.get(); }
void MainComponent::getAllCommands(juce::Array<juce::CommandID>& c) { commandHandler->getAllCommands(c); }
void MainComponent::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& r) { commandHandler->getCommandInfo(id, r); }
bool MainComponent::perform(const juce::ApplicationCommandTarget::InvocationInfo& i) { return commandHandler->perform(i); }