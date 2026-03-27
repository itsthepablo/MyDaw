#include "MainComponent.h"

MainComponent::MainComponent() {
    setupCommands();
    commandManager.registerAllCommandsForTarget(this);
    addKeyListener(commandManager.getKeyMappings());

    ui.resourceMeter = std::make_unique<ResourceMeter>(*this);

    UIManager::setupUI(ui, *this, [this] { closePianoRoll(); });

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

    // --- CONEXIÓN METRÓNOMO (UI -> DSP) ---
    ui.topMenuBar.metronomeBtn.onClick = [this] {
        audioEngine.metronome.setEnabled(ui.topMenuBar.metronomeBtn.getToggleState());
    };

    ui.playlistUI.onNewTrackRequested = [this](TrackType type) { ui.trackContainer.addTrack(type); };

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

    ui.toolbarButtons.onSnapChanged = [this](double s) { ui.playlistUI.snapPixels = s; };
    ui.toolbarButtons.onToolChanged = [this](int t) { ui.playlistUI.setTool(t); };

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
}

void MainComponent::setupBridges() {
    BridgeManager::initializeAllBridges({
        ui.trackContainer, ui.playlistUI, ui.pianoRollUI, ui.mixerUI, ui.effectsPanelUI,
        ui.instrumentPanelUI, // <-- SE AÑADE AQUÍ
        ui.transportBar, ui.topMenuBar, ui.toolbarButtons, ui.bottomDock, ui.leftSidebar, audioEngine, audioMutex,
        isBottomDockVisible, isLeftSidebarVisible,
        [this] { openPianoRoll(); }, [this] { closePianoRoll(); },
        [this] { resized(); }, [this] { toggleViewMode(); },
        [this] {
            if (isPianoRollVisible) closePianoRoll();
            currentView = ViewMode::Arrangement;
            isBottomDockVisible = true;
            ui.bottomDock.showTab(BottomDock::EffectsTab);
            resized();
        }
        });
}

void MainComponent::prepareToPlay(int samples, double s) {
    audioEngine.prepareToPlay(samples, s, ui.trackContainer, audioMutex);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& buffer) {
    audioEngine.processBlock(buffer, ui.trackContainer, ui.pianoRollUI, ui.playlistUI, ui.mixerUI, audioMutex);
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
    ProjectManager::loadProject(file, ui.trackContainer, audioMutex, ui.playlistUI, ui.pickerPanelUI, [this] { resized(); });
}

void MainComponent::saveProject() {
    ProjectManager::saveProject(ui.trackContainer, fileChooser);
}

void MainComponent::resized() {
    LayoutHandler::performLayout({
        getLocalBounds(), ui.topMenuBar, ui.hintPanel, ui.toolbarButtons, ui.resourceMeter.get(), ui.transportBar,
        ui.pianoRollUI, ui.closePianoRollBtn, ui.mixerUI, ui.masterChannelUI, ui.trackContainer, ui.playlistUI,
        ui.bottomDock, ui.bottomDockResizer, ui.leftSidebar, ui.sidebarResizer,
        currentView, isPianoRollVisible, isBottomDockVisible, bottomDockHeight, isLeftSidebarVisible, leftSidebarWidth
        });
}

juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget() { return commandHandler.get(); }
void MainComponent::getAllCommands(juce::Array<juce::CommandID>& c) { commandHandler->getAllCommands(c); }
void MainComponent::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& r) { commandHandler->getCommandInfo(id, r); }
bool MainComponent::perform(const juce::ApplicationCommandTarget::InvocationInfo& i) { return commandHandler->perform(i); }