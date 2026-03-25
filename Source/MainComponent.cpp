#include "MainComponent.h"
#include "Bridge/TrackPianoRollBridge.h"
#include "Bridge/TrackEffectsBridge.h"
#include "Bridge/TrackMixerPlaylistBridge.h"
#include "Bridge/TransportBridge.h" 
#include "Bridge/InterfaceBridge.h"

MainComponent::MainComponent() {
    commandManager.registerAllCommandsForTarget(this);
    addKeyListener(commandManager.getKeyMappings());

    resourceMeter = std::make_unique<ResourceMeter>(*this);

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
    masterChannelUI.setVisible(false); // Inicia oculto porque arrancamos en Arrangement

    closePianoRollBtn.setButtonText("Cerrar Piano Roll");
    closePianoRollBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(200, 70, 70));
    closePianoRollBtn.setTooltip("Vuelve a la vista de Playlist y restaura tus paneles.");
    closePianoRollBtn.onClick = [this] { closePianoRoll(); };

    topMenuBar.viewToggleBtn.onClick = [this] { toggleViewMode(); };

    // ENLAZAMOS EL BOTÓN DE GUARDAR DEL MENÚ
    topMenuBar.onSaveRequested = [this] { saveProject(); };

    bottomDock.setVisible(true);
    leftSidebar.setVisible(true);

    trackContainer.setExternalMutex(&audioMutex);

    playlistUI.setExternalMutex(&audioMutex);
    playlistUI.setTrackContainer(&trackContainer);
    pickerPanelUI.setTrackContainer(&trackContainer);

    playlistUI.onNewTrackRequested = [this](TrackType type) {
        trackContainer.addTrack(type);
        };

    pickerPanelUI.onDeleteRequested = [this](juce::String name, bool isMidi) {
        playlistUI.deleteClipsByName(name, isMidi);
        trackContainer.deleteUnusedClipsByName(name, isMidi);
        pickerPanelUI.refreshList();
        };

    leftSidebar.onClose = [this] { isLeftSidebarVisible = false; resized(); };
    bottomDock.onClose = [this] { isBottomDockVisible = false; resized(); };
    sidebarResizer.onResizeDelta = [this](int deltaX) { leftSidebarWidth = juce::jlimit(150, 600, leftSidebarWidth + deltaX); resized(); };
    bottomDockResizer.onResizeDelta = [this](int deltaY) { bottomDockHeight = juce::jlimit(100, (int)(getHeight() * 0.8f), bottomDockHeight - deltaY); resized(); };

    toolbarButtons.onSnapChanged = [this](double newSnapPixels) { playlistUI.snapPixels = newSnapPixels; };
    toolbarButtons.onToolChanged = [this](int toolId) { playlistUI.setTool(toolId); };

    playlistUI.onMidiClipDeleted = [this](MidiClipData* deletedClip) {
        if (pianoRollUI.getActiveClip() == deletedClip) {
            closePianoRoll();
        }
        };

    TrackPianoRollBridge::connect(trackContainer, playlistUI, pianoRollUI, [this] { openPianoRoll(); });
    TrackPianoRollBridge::connectPlaylist(playlistUI, pianoRollUI, [this] { openPianoRoll(); });

    TrackEffectsBridge::connect(trackContainer, effectsPanelUI, audioMutex,
        audioEngine.clock.sampleRate, audioEngine.clock.blockSize,
        [this] {
            if (isPianoRollVisible) closePianoRoll();
            currentView = ViewMode::Arrangement;
            isBottomDockVisible = true;
            bottomDock.showTab(BottomDock::EffectsTab);
            resized();
        });

    TrackMixerPlaylistBridge::connect(trackContainer, mixerUI, playlistUI);
    TransportBridge::connect(transportBar, pianoRollUI, playlistUI);

    InterfaceBridge::connect(toolbarButtons, isBottomDockVisible, isLeftSidebarVisible,
        bottomDock, effectsPanelUI, leftSidebar, trackContainer,
        [this] { resized(); },
        [this] { toggleViewMode(); });

    trackContainer.onDeleteTrack = [this](int index) {
        const juce::ScopedLock sl(audioMutex);
        if (index >= 0 && index < trackContainer.getTracks().size()) {
            Track* trackToDelete = trackContainer.getTracks()[index];
            TrackPianoRollBridge::cleanup(pianoRollUI, [this] { closePianoRoll(); }, trackToDelete);

            playlistUI.purgeClipsOfTrack(trackToDelete);
            trackContainer.removeTrack(index);

            playlistUI.updateScrollBars();
            playlistUI.repaint();
            pickerPanelUI.refreshList();

            if (bottomDock.getCurrentTab() == BottomDock::EffectsTab) {
                effectsPanelUI.setTrack(nullptr);
            }
            resized();
        }
        };

    setSize(1200, 800);
    setAudioChannels(0, 2);
}

MainComponent::~MainComponent() { shutdownAudio(); }

void MainComponent::prepareToPlay(int samples, double s) { audioEngine.prepareToPlay(samples, s, trackContainer, audioMutex); }
void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) { audioEngine.processBlock(bufferToFill, trackContainer, pianoRollUI, playlistUI, mixerUI, audioMutex); }
void MainComponent::releaseResources() { audioEngine.releaseResources(); }

void MainComponent::paint(juce::Graphics& g) { g.fillAll(juce::Colour(30, 32, 35)); }

void MainComponent::toggleViewMode() {
    currentView = (currentView == ViewMode::Arrangement) ? ViewMode::Mixer : ViewMode::Arrangement;
    resized();
}

bool MainComponent::keyPressed(const juce::KeyPress& key) {
    if (key.getKeyCode() == juce::KeyPress::tabKey) {
        toggleViewMode();
        return true;
    }
    return false;
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
        pianoRollUI.setActiveClip(nullptr);
        resized();
    }
}

// === FUNCIONES QUE EL LINKER ESTABA BUSCANDO ===
void MainComponent::loadProject(const juce::File& file) {
    if (!file.existsAsFile()) return;

    auto xml = juce::XmlDocument::parse(file);
    if (xml != nullptr) {
        if (xml->hasTagName("PERRITOGORDO_PROJECT")) {
            const juce::ScopedLock sl(audioMutex);

            while (trackContainer.getTracks().size() > 0) {
                trackContainer.removeTrack(0);
            }

            if (auto* tracksTree = xml->getChildByName("TRACK_LIST")) {
                for (auto* tXml : tracksTree->getChildIterator()) {
                    if (tXml->hasTagName("TRACK")) {
                        trackContainer.addTrack(TrackType::Audio);
                    }
                }
            }

            playlistUI.updateScrollBars();
            playlistUI.repaint();
            pickerPanelUI.refreshList();
            resized();
        }
    }
}

void MainComponent::saveProject() {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Guardar Proyecto (.perritogordo)",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.perritogordo");

    auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc) {
        auto file = fc.getResult();

        if (file != juce::File()) {
            if (file.getFileExtension() != ".perritogordo")
                file = file.withFileExtension(".perritogordo");

            juce::ValueTree projectTree("PERRITOGORDO_PROJECT");
            projectTree.setProperty("dawVersion", ProjectInfo::versionString, nullptr);
            projectTree.setProperty("projectName", file.getFileNameWithoutExtension(), nullptr);

            juce::ValueTree settings("SETTINGS");
            settings.setProperty("bpm", 120.0, nullptr);
            projectTree.addChild(settings, -1, nullptr);

            juce::ValueTree tracksTree("TRACK_LIST");
            for (auto* track : trackContainer.getTracks()) {
                juce::ValueTree t("TRACK");
                t.setProperty("name", track->getName(), nullptr);
                tracksTree.addChild(t, -1, nullptr);
            }
            projectTree.addChild(tracksTree, -1, nullptr);

            if (auto xml = projectTree.createXml()) {
                xml->writeTo(file);
            }
        }
        });
}
// ===============================================

void MainComponent::resized() {
    auto area = getLocalBounds();

    topMenuBar.setBounds(area.removeFromTop(30));
    hintPanel.setBounds(area.removeFromBottom(28));

    auto topArea = area.removeFromTop(45);
    toolbarButtons.setBounds(topArea.removeFromRight(550));

    if (resourceMeter != nullptr) {
        resourceMeter->setBounds(topArea.removeFromRight(100).reduced(8, 10));
    }
    transportBar.setBounds(topArea);

    if (isPianoRollVisible) {
        pianoRollUI.setVisible(true);
        closePianoRollBtn.setVisible(true);

        pianoRollUI.setBounds(area);
        closePianoRollBtn.setBounds(area.getRight() - 160, area.getY() + 10, 140, 25);

        trackContainer.setVisible(false);
        playlistUI.setVisible(false);
        bottomDock.setVisible(false);
        bottomDockResizer.setVisible(false);
        leftSidebar.setVisible(false);
        sidebarResizer.setVisible(false);
        mixerUI.setVisible(false);
        masterChannelUI.setVisible(false);
    }
    else {
        pianoRollUI.setVisible(false);
        closePianoRollBtn.setVisible(false);

        if (currentView == ViewMode::Mixer) {
            masterChannelUI.setVisible(true); // Se muestra SOLO en el Mixer

            trackContainer.setVisible(false);
            playlistUI.setVisible(false);
            bottomDock.setVisible(false);
            bottomDockResizer.setVisible(false);
            leftSidebar.setVisible(false);
            sidebarResizer.setVisible(false);

            mixerUI.setVisible(true);

            // MASTER AL LADO IZQUIERDO EN EL MIXER
            float scale = area.getHeight() / 600.0f;
            int masterWidth = juce::roundToInt(120.0f * scale);
            masterChannelUI.setBounds(area.removeFromLeft(masterWidth));

            mixerUI.setBounds(area);
        }
        else {
            masterChannelUI.setVisible(false); // Oculto en el Arrangement
            mixerUI.setVisible(false);

            trackContainer.setVisible(true);
            playlistUI.setVisible(true);

            if (isBottomDockVisible) {
                bottomDock.setVisible(true);
                bottomDockResizer.setVisible(true);
                bottomDock.setBounds(area.removeFromBottom(bottomDockHeight));
                bottomDockResizer.setBounds(area.removeFromBottom(4));
            }
            else {
                bottomDock.setVisible(false);
                bottomDockResizer.setVisible(false);
            }

            if (isLeftSidebarVisible) {
                leftSidebar.setVisible(true);
                sidebarResizer.setVisible(true);
                leftSidebar.setBounds(area.removeFromLeft(leftSidebarWidth));
                sidebarResizer.setBounds(area.removeFromLeft(4));
            }
            else {
                leftSidebar.setVisible(false);
                sidebarResizer.setVisible(false);
            }

            trackContainer.setBounds(area.removeFromLeft(250));
            playlistUI.setBounds(area);
        }
    }
}

juce::ApplicationCommandTarget* MainComponent::getNextCommandTarget() { return nullptr; }
void MainComponent::getAllCommands(juce::Array<juce::CommandID>& c) { c.add(playStopCommand); }

void MainComponent::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& result) {
    if (id == playStopCommand) {
        result.setInfo("Play / Stop", "Transporte", "Transport", 0);
        result.addDefaultKeypress(' ', 0);
    }
}

bool MainComponent::perform(const juce::ApplicationCommandTarget::InvocationInfo& info) {
    if (info.commandID == playStopCommand) {
        transportBar.playBtn.triggerClick();
        return true;
    }
    return false;
}