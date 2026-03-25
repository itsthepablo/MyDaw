#include "MainComponent.h"

MainComponent::MainComponent() {
    setupCommands(); // Configura atajos antes de los listeners
    commandManager.registerAllCommandsForTarget(this);
    addKeyListener(commandManager.getKeyMappings());

    resourceMeter = std::make_unique<ResourceMeter>(*this);

    setupUIComponents();
    setupCallbacks();
    setupBridges();

    trackContainer.setExternalMutex(&audioMutex);
    playlistUI.setExternalMutex(&audioMutex);
    playlistUI.setTrackContainer(&trackContainer);
    pickerPanelUI.setTrackContainer(&trackContainer);

    setSize(1200, 800);
    setAudioChannels(0, 2);
}

MainComponent::~MainComponent() { shutdownAudio(); }

void MainComponent::setupCommands() {
    commandHandler = std::make_unique<DAWCommandHandler>(CommandActions{
        [this] { transportBar.playBtn.triggerClick(); },
        [this] { toggleViewMode(); }
    });
}

void MainComponent::setupUIComponents() {
    addAndMakeVisible(topMenuBar);
    addAndMakeVisible(hintPanel);
    addAndMakeVisible(transportBar);
    addAndMakeVisible(toolbarButtons);
    addAndMakeVisible(*resourceMeter);
    addAndMakeVisible(leftSidebar);
    addAndMakeVisible(sidebarResizer);
    addAndMakeVisible(trackContainer);
    addAndMakeVisible(playlistUI);
    addAndMakeVisible(bottomDockResizer);
    addAndMakeVisible(bottomDock);
    addAndMakeVisible(mixerUI);
    addAndMakeVisible(masterChannelUI);
    addAndMakeVisible(pianoRollUI);
    addAndMakeVisible(closePianoRollBtn);

    pianoRollUI.setVisible(false);
    closePianoRollBtn.setVisible(false);
    mixerUI.setVisible(false);
    masterChannelUI.setVisible(false);

    closePianoRollBtn.setButtonText("Cerrar Piano Roll");
    closePianoRollBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(200, 70, 70));
    closePianoRollBtn.onClick = [this] { closePianoRoll(); };

    bottomDock.setVisible(true);
    leftSidebar.setVisible(true);
}

void MainComponent::setupCallbacks() {
    topMenuBar.viewToggleBtn.onClick = [this] { toggleViewMode(); };
    topMenuBar.onSaveRequested = [this] { saveProject(); };

    playlistUI.onNewTrackRequested = [this](TrackType type) { trackContainer.addTrack(type); };
    pickerPanelUI.onDeleteRequested = [this](juce::String name, bool isMidi) {
        playlistUI.deleteClipsByName(name, isMidi);
        trackContainer.deleteUnusedClipsByName(name, isMidi);
        pickerPanelUI.refreshList();
    };

    leftSidebar.onClose = [this] { isLeftSidebarVisible = false; resized(); };
    bottomDock.onClose = [this] { isBottomDockVisible = false; resized(); };
    
    sidebarResizer.onResizeDelta = [this](int d) { leftSidebarWidth = juce::jlimit(150, 600, leftSidebarWidth + d); resized(); };
    bottomDockResizer.onResizeDelta = [this](int d) { bottomDockHeight = juce::jlimit(100, (int)(getHeight() * 0.8f), bottomDockHeight - d); resized(); };

    toolbarButtons.onSnapChanged = [this](double s) { playlistUI.snapPixels = s; };
    toolbarButtons.onToolChanged = [this](int t) { playlistUI.setTool(t); };

    playlistUI.onMidiClipDeleted = [this](MidiClipData* c) { if (pianoRollUI.getActiveClip() == c) closePianoRoll(); };

    trackContainer.onDeleteTrack = [this](int index) {
        const juce::ScopedLock sl(audioMutex);
        if (index >= 0 && index < trackContainer.getTracks().size()) {
            Track* trackToDelete = trackContainer.getTracks()[index];
            BridgeManager::cleanupTrack(trackToDelete, pianoRollUI, [this] { closePianoRoll(); });
            playlistUI.purgeClipsOfTrack(trackToDelete);
            trackContainer.removeTrack(index);
            playlistUI.updateScrollBars();
            playlistUI.repaint();
            pickerPanelUI.refreshList();
            if (bottomDock.getCurrentTab() == BottomDock::EffectsTab) effectsPanelUI.setTrack(nullptr);
            resized();
        }
    };
}

void MainComponent::setupBridges() {
    BridgeManager::initializeAllBridges({
        trackContainer, playlistUI, pianoRollUI, mixerUI, effectsPanelUI,
        transportBar, toolbarButtons, bottomDock, leftSidebar, audioEngine, audioMutex,
        isBottomDockVisible, isLeftSidebarVisible,
        [this] { openPianoRoll(); }, [this] { closePianoRoll(); },
        [this] { resized(); }, [this] { toggleViewMode(); },
        [this] { 
            if (isPianoRollVisible) closePianoRoll();
            currentView = ViewMode::Arrangement;
            isBottomDockVisible = true;
            bottomDock.showTab(BottomDock::EffectsTab);
            resized();
        }
    });
}

void MainComponent::prepareToPlay(int samples, double s) { audioEngine.prepareToPlay(samples, s, trackContainer, audioMutex); }
void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& buffer) { audioEngine.processBlock(buffer, trackContainer, pianoRollUI, playlistUI, mixerUI, audioMutex); }
void MainComponent::releaseResources() { audioEngine.releaseResources(); }
void MainComponent::paint(juce::Graphics& g) { g.fillAll(juce::Colour(30, 32, 35)); }

void MainComponent::toggleViewMode() {
    currentView = (currentView == ViewMode::Arrangement) ? ViewMode::Mixer : ViewMode::Arrangement;
    resized();
}

void MainComponent::openPianoRoll() {
    if (!isPianoRollVisible) {
        prePianoRollLeftSidebar = isLeftSidebarVisible; prePianoRollBottomDock = isBottomDockVisible;
        isPianoRollVisible = true; currentView = ViewMode::Arrangement; resized();
    }
}

void MainComponent::closePianoRoll() {
    if (isPianoRollVisible) {
        isPianoRollVisible = false; isLeftSidebarVisible = prePianoRollLeftSidebar;
        isBottomDockVisible = prePianoRollBottomDock; pianoRollUI.setActiveClip(nullptr); resized();
    }
}

void MainComponent::loadProject(const juce::File& file) { ProjectManager::loadProject(file, trackContainer, audioMutex, playlistUI, pickerPanelUI, [this] { resized(); }); }
void MainComponent::saveProject() { ProjectManager::saveProject(trackContainer, fileChooser); }

void MainComponent::resized() {
    LayoutHandler::performLayout({
        getLocalBounds(), topMenuBar, hintPanel, toolbarButtons, resourceMeter.get(), transportBar,
        pianoRollUI, closePianoRollBtn, mixerUI, masterChannelUI, trackContainer, playlistUI,
        bottomDock, bottomDockResizer, leftSidebar, sidebarResizer,
        currentView, isPianoRollVisible, isBottomDockVisible, bottomDockHeight, isLeftSidebarVisible, leftSidebarWidth
    });
}

// Delegación obligatoria al handler
juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget() { return commandHandler.get(); }
void MainComponent::getAllCommands(juce::Array<juce::CommandID>& c) { commandHandler->getAllCommands(c); }
void MainComponent::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& r) { commandHandler->getCommandInfo(id, r); }
bool MainComponent::perform(const juce::ApplicationCommandTarget::InvocationInfo& i) { return commandHandler->perform(i); }