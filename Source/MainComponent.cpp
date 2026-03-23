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

    // --- AÑADIMOS LA BARRA DE ESTADO INFERIOR ---
    addAndMakeVisible(hintPanel);

    // --- BARRA SUPERIOR ---
    addAndMakeVisible(transportBar);
    addAndMakeVisible(toolbarButtons);
    addAndMakeVisible(*resourceMeter);

    // --- PANELES LATERALES E INFERIORES ---
    addAndMakeVisible(leftSidebar);
    addAndMakeVisible(sidebarResizer);
    addAndMakeVisible(trackContainer);
    addAndMakeVisible(playlistUI);

    // Añadimos el Dock y su barra resizer
    addAndMakeVisible(bottomDockResizer);
    addAndMakeVisible(bottomDock);

    bottomDock.setVisible(true);
    leftSidebar.setVisible(true);

    trackContainer.setExternalMutex(&audioMutex);
    playlistUI.setExternalMutex(&audioMutex);

    pickerPanelUI.setTrackContainer(&trackContainer);

    // --- LÓGICA DE LOS BOTONES CERRAR [X] ---
    leftSidebar.onClose = [this] {
        isLeftSidebarVisible = false;
        resized();
        };

    bottomDock.onClose = [this] {
        isBottomDockVisible = false;
        resized();
        };

    // --- LÓGICA DE REDIMENSIONAMIENTO LATERAL (Izquierdo) ---
    sidebarResizer.onResizeDelta = [this](int deltaX) {
        leftSidebarWidth = juce::jlimit(150, 600, leftSidebarWidth + deltaX);
        resized();
        };

    // --- LÓGICA DE REDIMENSIONAMIENTO INFERIOR ---
    bottomDockResizer.onResizeDelta = [this](int deltaY) {
        // Recordar: Arrastrar hacia ARRIBA es deltaY NEGATIVO.
        // Queremos que el panel CREZCA al arrastrar hacia arriba, por lo que RESTAMOS deltaY.

        // jlimit: mínimo 100px (para ver pestañas), máximo 80% del alto total de la ventana.
        int maxHeight = (int)(getHeight() * 0.8f);
        bottomDockHeight = juce::jlimit(100, maxHeight, bottomDockHeight - deltaY);

        resized(); // Forzamos repintado instantáneo
        };

    // Conexiones de la barra superior
    toolbarButtons.onSnapChanged = [this](double newSnapPixels) { playlistUI.snapPixels = newSnapPixels; };
    toolbarButtons.onToolChanged = [this](int toolId) { playlistUI.setTool(toolId); };

    playlistUI.onMidiClipDeleted = [this](MidiClipData* deletedClip) {
        if (pianoRollUI.getActiveNotesPointer() == &(deletedClip->notes)) {
            pianoRollUI.setActiveNotes(nullptr);
            if (pianoRollWindow) pianoRollWindow->setVisible(false);
        }
        };

    // Puentes de comunicación
    TrackPianoRollBridge::connect(trackContainer, playlistUI, pianoRollUI, pianoRollWindow);
    TrackPianoRollBridge::connectPlaylist(playlistUI, pianoRollUI, pianoRollWindow);

    TrackEffectsBridge::connect(trackContainer, effectsPanelUI, audioMutex,
        audioEngine.clock.sampleRate, audioEngine.clock.blockSize,
        [this] {
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
            TrackPianoRollBridge::cleanup(pianoRollUI, pianoRollWindow, trackContainer.getTracks()[index]);
            trackContainer.removeTrack(index);
            playlistUI.updateScrollBars();
            playlistUI.repaint();

            // Si borramos una pista mientras veíamos sus efectos, cerramos la pestaña
            if (leftSidebar.getCurrentTab() == LeftSidebar::FxTab) isLeftSidebarVisible = false;
            resized();
        }
        };

    setSize(1200, 800);
    setAudioChannels(0, 2);
}

MainComponent::~MainComponent() {
    shutdownAudio();
}

void MainComponent::prepareToPlay(int samples, double s) {
    audioEngine.prepareToPlay(samples, s, trackContainer, audioMutex);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) {
    audioEngine.processBlock(bufferToFill, trackContainer, pianoRollUI, playlistUI, mixerUI, audioMutex);
}

void MainComponent::releaseResources() {
    audioEngine.releaseResources();
}

void MainComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(30, 32, 35));
}

void MainComponent::resized() {
    auto area = getLocalBounds();

    // --- 1. BARRA DE ESTADO INFERIOR (HINT PANEL) ---
    // Le quitamos 28 píxeles de la base absoluta del DAW
    hintPanel.setBounds(area.removeFromBottom(28));

    // --- 2. BARRA SUPERIOR (TOP BAR) ---
    auto topArea = area.removeFromTop(45);

    // Ahora la barra de transporte puede tomar todo el lado izquierdo
    toolbarButtons.setBounds(topArea.removeFromRight(550));
    resourceMeter->setBounds(topArea.removeFromRight(100).reduced(8, 10));
    transportBar.setBounds(topArea); // Ocupa todo el centro y la izquierda

    // --- 3. CONTENEDOR INFERIOR (MIXER + RACK + RESIZER) ---
    // (Aparece justo encima del Hint Panel porque ya le restamos ese espacio a 'area')
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

    // --- 4. CONTENEDOR LATERAL IZQUIERDO ---
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

    // --- 5. ZONA CENTRAL ---
    trackContainer.setBounds(area.removeFromLeft(250));
    playlistUI.setBounds(area);
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