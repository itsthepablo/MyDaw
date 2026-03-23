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

    // --- NUEVO: Piano Roll y Botón de Volver ---
    addAndMakeVisible(pianoRollUI);
    addAndMakeVisible(closePianoRollBtn);

    pianoRollUI.setVisible(false); // Oculto por defecto
    closePianoRollBtn.setVisible(false);

    closePianoRollBtn.setButtonText("Cerrar Piano Roll");
    closePianoRollBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(200, 70, 70)); // Rojo destacado
    closePianoRollBtn.setTooltip("Vuelve a la vista de Playlist y restaura tus paneles.");
    closePianoRollBtn.onClick = [this] { closePianoRoll(); };

    bottomDock.setVisible(true);
    leftSidebar.setVisible(true);

    trackContainer.setExternalMutex(&audioMutex);
    playlistUI.setExternalMutex(&audioMutex);
    pickerPanelUI.setTrackContainer(&trackContainer);

    leftSidebar.onClose = [this] { isLeftSidebarVisible = false; resized(); };
    bottomDock.onClose = [this] { isBottomDockVisible = false; resized(); };
    sidebarResizer.onResizeDelta = [this](int deltaX) { leftSidebarWidth = juce::jlimit(150, 600, leftSidebarWidth + deltaX); resized(); };
    bottomDockResizer.onResizeDelta = [this](int deltaY) { bottomDockHeight = juce::jlimit(100, (int)(getHeight() * 0.8f), bottomDockHeight - deltaY); resized(); };

    toolbarButtons.onSnapChanged = [this](double newSnapPixels) { playlistUI.snapPixels = newSnapPixels; };
    toolbarButtons.onToolChanged = [this](int toolId) { playlistUI.setTool(toolId); };

    playlistUI.onMidiClipDeleted = [this](MidiClipData* deletedClip) {
        if (pianoRollUI.getActiveNotesPointer() == &(deletedClip->notes)) {
            closePianoRoll(); // Cerramos si borramos el clip activo
        }
        };

    // --- CONEXIONES MODIFICADAS PARA LLAMAR AL MODO ENFOQUE ---
    TrackPianoRollBridge::connect(trackContainer, playlistUI, pianoRollUI, [this] { openPianoRoll(); });
    TrackPianoRollBridge::connectPlaylist(playlistUI, pianoRollUI, [this] { openPianoRoll(); });

    TrackEffectsBridge::connect(trackContainer, effectsPanelUI, audioMutex,
        audioEngine.clock.sampleRate, audioEngine.clock.blockSize,
        [this] {
            // Si abrimos un efecto, nos aseguramos de salir del Piano Roll para verlo
            if (isPianoRollVisible) closePianoRoll();
            isLeftSidebarVisible = true;
            leftSidebar.showTab(LeftSidebar::FxTab);
            resized();
        });

    TrackMixerPlaylistBridge::connect(trackContainer, mixerUI, playlistUI);
    TransportBridge::connect(transportBar, pianoRollUI, playlistUI);
    InterfaceBridge::connect(toolbarButtons, isBottomDockVisible, isLeftSidebarVisible,
        bottomDock, effectsPanelUI, leftSidebar, trackContainer, [this] { resized(); });

    trackContainer.onDeleteTrack = [this](int index) {
        const juce::ScopedLock sl(audioMutex);
        if (index >= 0 && index < trackContainer.getTracks().size()) {
            TrackPianoRollBridge::cleanup(pianoRollUI, [this] { closePianoRoll(); }, trackContainer.getTracks()[index]);
            trackContainer.removeTrack(index);
            playlistUI.updateScrollBars();
            playlistUI.repaint();
            if (leftSidebar.getCurrentTab() == LeftSidebar::FxTab) isLeftSidebarVisible = false;
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

// --- LÓGICA DE APERTURA Y CIERRE DEL MODO ENFOQUE ---
void MainComponent::openPianoRoll() {
    if (!isPianoRollVisible) {
        // Guardamos cómo tenía el usuario su DAW antes de entrar
        prePianoRollLeftSidebar = isLeftSidebarVisible;
        prePianoRollBottomDock = isBottomDockVisible;
        isPianoRollVisible = true;
        resized();
    }
}

void MainComponent::closePianoRoll() {
    if (isPianoRollVisible) {
        isPianoRollVisible = false;
        // Restauramos los paneles tal cual estaban
        isLeftSidebarVisible = prePianoRollLeftSidebar;
        isBottomDockVisible = prePianoRollBottomDock;
        pianoRollUI.setActiveNotes(nullptr);
        resized();
    }
}

void MainComponent::resized() {
    auto area = getLocalBounds();

    // 1. Barras maestras inamovibles (Cabecera y Hint Panel)
    topMenuBar.setBounds(area.removeFromTop(30));
    hintPanel.setBounds(area.removeFromBottom(28));

    // 2. Barra de Transporte (Play/Stop siempre debe verse)
    auto topArea = area.removeFromTop(45);
    toolbarButtons.setBounds(topArea.removeFromRight(550));
    resourceMeter->setBounds(topArea.removeFromRight(100).reduced(8, 10));
    transportBar.setBounds(topArea);

    if (isPianoRollVisible) {
        // =======================================================
        // MODO ENFOQUE: PIANO ROLL OCUPA TODO EL RESTO
        // =======================================================
        pianoRollUI.setVisible(true);
        closePianoRollBtn.setVisible(true);

        // El Piano Roll ocupa todo el espacio central masivo
        pianoRollUI.setBounds(area);

        // Flotamos el botón de cerrar en la esquina superior derecha del Piano Roll
        closePianoRollBtn.setBounds(area.getRight() - 160, area.getY() + 10, 140, 25);

        // Ocultamos todo lo demás
        trackContainer.setVisible(false);
        playlistUI.setVisible(false);
        bottomDock.setVisible(false);
        bottomDockResizer.setVisible(false);
        leftSidebar.setVisible(false);
        sidebarResizer.setVisible(false);
    }
    else {
        // =======================================================
        // MODO PLAYLIST (NORMAL)
        // =======================================================
        pianoRollUI.setVisible(false);
        closePianoRollBtn.setVisible(false);

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