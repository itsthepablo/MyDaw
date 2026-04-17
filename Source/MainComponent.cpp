#include "MainComponent.h"

MainComponent::MainComponent() {
    setupCommands();
    commandController->getCommandManager().registerAllCommandsForTarget(this);
    addKeyListener(commandController->getCommandManager().getKeyMappings());

    ui.resourceMeter = std::make_unique<ResourceMeter>(*this);

    UIManager::setupUI(ui, *this, [this] { viewManager.closePianoRoll(); });
    viewManager.setup();
    ui.onResized = [this] { resized(); };

    // Registrar Analizador y VU Meter en el Engine
    audioEngine.analyzerToFeed.store(&ui.inspectorPanelUI.analyzer);
    audioEngine.vuToFeed.store(&ui.vuMeterUI.getEngine());

    setupCallbacks();
    setupBridges();

    ui.trackContainer.setExternalMutex(&audioMutex);
    ui.playlistUI.setExternalMutex(&audioMutex);
    ui.mixerUI.setAudioMutex(&audioMutex);
    ui.miniMixerUI.setAudioMutex(&audioMutex);
    ui.playlistUI.setTrackContainer(&ui.trackContainer);
    ui.pickerPanelUI.setTrackContainer(&ui.trackContainer);

    setSize(1920, 1080);

    setAudioChannels(0, 2);
}

MainComponent::~MainComponent() { shutdownAudio(); }

void MainComponent::setupCommands() {
    commandController = std::make_unique<CommandController>(ui, viewManager, audioEngine);
}

void MainComponent::setupCallbacks() {
    ui.topMenuBar.viewToggleBtn.onClick = [this] { viewManager.toggleViewMode(); };
    ui.topMenuBar.onSaveRequested = [this] { projectController.saveProject(); };
    ui.topMenuBar.onExportRequested = [this] { renderController.startExport(this); };
    ui.topMenuBar.onThemeManagerRequested = [this] {
        if (themeWindow == nullptr)
            themeWindow = std::make_unique<ThemeManagerWindow>("Theme Manager");
        themeWindow->setVisible(true);
        themeWindow->toFront(true);
    };
    ui.topMenuBar.onThemeManagerRequested = [this] {
        if (themeWindow == nullptr)
            themeWindow = std::make_unique<ThemeManagerWindow>("Theme Manager");
        themeWindow->setVisible(true);
        themeWindow->toFront(true);
    };
    
    ui.topMenuBar.onExportStemsRequested = [this] { renderController.showStemRenderer(this); };

    // --- CONEXIÓN METRÓNOMO (UI -> DSP) ---
    ui.topMenuBar.metronomeBtn.onClick = [this] {
        audioEngine.metronome.setEnabled(ui.topMenuBar.metronomeBtn.getToggleState());
        };
    ui.trackContainer.onToggleAnalysisTrack = [this](TrackType type, bool visible) {
        trackManager.handleToggleAnalysisTrack(type, visible);
    };

    ui.topMenuBar.onNewTrackRequested = [this](TrackType type) { ui.trackContainer.addTrack(type); };
    ui.playlistUI.onNewTrackRequested = [this](TrackType type) { ui.trackContainer.addTrack(type); };

    ui.playlistUI.onVerticalScroll = [this](int scrollPos) {
        ui.trackContainer.setVOffset(scrollPos);
        };

    ui.playlistUI.onZoomChanged = [this](float zoom) {
        ui.topMenuBar.setZoomInfo((double)zoom);
    };
    ui.topMenuBar.setZoomInfo(ui.playlistUI.hZoom);

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
    };

    // --- CONEXIÓN MASTER TRACK ---
    ui.masterStrip.onTrackSelected = [this](Track* mt) {
        selectionManager.selectTrack(mt, true);
    };

    ui.masterStrip.onEffectsRequested = [this](Track* mt) {
        selectionManager.selectTrack(mt, true);
        viewManager.showBottomDock(BottomDock::EffectsTab);
    };


    ui.trackContainer.onOpenEffects = [this](Track& track) {
        selectionManager.selectTrack(&track, false);
        viewManager.showBottomDock(BottomDock::EffectsTab);
    };

    ui.trackContainer.onTrackAdded = [this] {
        trackManager.handleTrackAdded(ui.trackContainer.getTracks().getLast());
    };

    ui.trackContainer.onTracksReordered = [this] {
        trackManager.handleTracksReordered();
    };


    ui.pickerPanelUI.onDeleteRequested = [this](juce::String name, bool isMidi) {
        ui.playlistUI.deleteClipsByName(name, isMidi);
        ui.trackContainer.deleteUnusedClipsByName(name, isMidi);
        ui.pickerPanelUI.refreshList();
        };

    ui.playlistUI.onMidiClipDeleted = [this](MidiPattern* c) {
        if (ui.pianoRollUI.getActiveClip() == c) viewManager.closePianoRoll();
        };

    ui.trackContainer.onDeleteTrack = [this](int index) {
        trackManager.handleDeleteTrack(index, [this] { viewManager.closePianoRoll(); });
    };

    // --- CONEXIÓN SIDECHAIN (DATA SOURCE) ---
    ui.effectsPanelUI.getAvailableTracks = [this]() -> juce::Array<Track*> {
        juce::Array<Track*> trackList;
        for (auto* t : ui.trackContainer.getTracks()) 
            trackList.add(t);
        return trackList;
    };

    // --- CONEXIÓN RIGHT DOCK (SELECTED TRACK PANEL) ---
    ui.rightDock.selectedTrackPanel.onAddVST3 = [this](Track& t) { ui.mixerUI.onAddVST3(t); };
    ui.rightDock.selectedTrackPanel.onAddNativeUtility = [this](Track& t) { ui.mixerUI.onAddNativeUtility(t); };
    ui.rightDock.selectedTrackPanel.onAddSend = [this](Track& t) { ui.mixerUI.onAddSend(t); };
    ui.rightDock.selectedTrackPanel.onOpenPlugin = [this](Track& t, int i) { ui.mixerUI.onOpenPlugin(t, i); };
    ui.rightDock.selectedTrackPanel.onDeleteEffect = [this](Track& t, int i) { ui.mixerUI.onDeleteEffect(t, i); };
    ui.rightDock.selectedTrackPanel.onDeleteSend = [this](Track& t, int i) { ui.mixerUI.onDeleteSend(t, i); };
    ui.rightDock.selectedTrackPanel.onBypassChanged = [this](Track& t, int i, bool b) { ui.mixerUI.onBypassChanged(t, i, b); };
    ui.rightDock.selectedTrackPanel.onCreateAutomation = [this](Track& t, int i, juce::String n) { ui.mixerUI.onCreateAutomation(t, i, n); };

    ui.trackContainer.onActiveTrackChanged = [this](Track* t) {
        selectionManager.selectTrack(t, false);
        ui.modulatorRackPanelUI.setTrack(t);
    };

    ui.trackContainer.onDeselectMasterRequested = [this] {
        if (ui.masterTrackObj) ui.masterTrackObj->isSelected = false;
        ui.masterStrip.repaint();
    };

    ui.rightDock.onLayoutChanged = [this] {
        resized();
    };
}

void MainComponent::setupBridges() {
    ui.playlistUI.setMasterTrack(ui.masterTrackObj.get());
    BridgeManager::initializeAllBridges({
        ui.trackContainer, ui.playlistUI, ui.pianoRollUI, ui.mixerUI, ui.miniMixerUI, 
        ui.masterChannelUI.get(), ui.effectsPanelUI,
        ui.instrumentPanelUI, 
        ui.topMenuBar, ui.bottomDock, ui.leftSidebar, audioEngine, audioMutex,
        viewManager.getBottomDockVisibleRef(), viewManager.getLeftSidebarVisibleRef(),
        [this] { viewManager.openPianoRoll(); }, [this] { viewManager.closePianoRoll(); },
        [this] { viewManager.resized(); }, [this] { viewManager.toggleViewMode(); },
        [this] {
            if (viewManager.isPianoRollVisibleNow()) viewManager.closePianoRoll();
            viewManager.showBottomDock(BottomDock::EffectsTab);
        },
        ui.masterTrackObj.get()
        });
}

void MainComponent::prepareToPlay(int samples, double s) {
    audioEngine.prepareToPlay(samples, s);
    ui.inspectorPanelUI.setSampleRate(s);
    ui.vuMeterUI.getEngine().setSampleRate(s);

    trackManager.prepareTracks(s, samples);
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
    
    // Solo si el Piano Roll está visible, forzamos su loop dinámico de patrón
    float finalLoopEnd = viewManager.isPianoRollVisibleNow() ? ui.pianoRollUI.getLoopEndPos() : ui.playlistUI.getLoopEndPos();
    audioEngine.transportState.loopEndPos.store(finalLoopEnd, std::memory_order_relaxed);

    // PROTECCIÓN ANTI-CRASH: El audioMutex se toma brevemente para garantizar que
    // el audio thread no acceda a Track* que el UI thread acaba de destruir.
    // El ScopedTryLock evita bloquear: si no puede adquirirlo, produce silencio
    // en vez de crashear. El UI thread solo lo toma durante addTrack/removeTrack.
    // Capturar latencia del hardware para sincronía UI/Audio
    if (auto* device = deviceManager.getCurrentAudioDevice()) {
        audioEngine.transportState.currentGlobalLatency.store(device->getOutputLatencyInSamples(), std::memory_order_relaxed);
    }

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

void MainComponent::paint(juce::Graphics& g) {
    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
        g.fillAll(theme->getSkinColor("WINDOW_BG", juce::Colour(28, 30, 34)));
    } else {
        g.fillAll(juce::Colour(28, 30, 34));
    }
}






void MainComponent::resized() {
    viewManager.resized();
}


