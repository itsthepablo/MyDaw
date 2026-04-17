#include "ViewManager.h"

ViewManager::ViewManager(DAWUIComponents& uiComponents, juce::Component& parentComponent)
    : ui(uiComponents), parent(parentComponent)
{
}

void ViewManager::setup() {
    // 1. Callbacks de Cierre de Docks
    ui.leftSidebar.onClose = [this] { isLeftSidebarVisible = false; resized(); };
    ui.bottomDock.onClose = [this] { isBottomDockVisible = false; resized(); };

    // 2. Callback de Cambio de Pestaña (Mini Mixer vs Otros)
    ui.bottomDock.onTabChanged = [this](BottomDock::Tab tab) {
        if (tab == BottomDock::MixerTab) {
            isResizerActive = true;
            bottomDockHeight = lastMixerHeight;
        } else {
            isResizerActive = false;
            bottomDockHeight = 300;
        }
        ui.bottomDockResizer.setVisible(isResizerActive);
        resized();
    };

    // 3. Callbacks de Resizers
    ui.sidebarResizer.onResizeDelta = [this](int d) {
        leftSidebarWidth = juce::jlimit(150, 600, leftSidebarWidth + d);
        resized();
    };

    ui.bottomDockResizer.onResizeDelta = [this](int d) {
        if (isResizerActive) {
            // d es el desplazamiento del ratón. Hacia abajo es positivo.
            // Queremos que hacia ARRIBA crezca la altura.
            lastMixerHeight = juce::jlimit(300, 800, lastMixerHeight - d);
            bottomDockHeight = lastMixerHeight;
            resized();
        }
    };
    
    // Sincronizar estado inicial de visibilidad del resizer
    ui.bottomDockResizer.setVisible(isResizerActive);
}

void ViewManager::resized() {
    updateLayout();
}

void ViewManager::updateLayout() {
    LayoutHandler::performLayout({
        parent.getLocalBounds(), ui.topMenuBar, ui.hintPanel, ui.resourceMeter.get(),
        ui.pianoRollUI, ui.closePianoRollBtn, ui.mixerUI, *ui.masterChannelUI, ui.trackContainer, ui.playlistUI,
        ui.bottomDock, ui.bottomDockResizer, ui.leftSidebar, ui.sidebarResizer, ui.masterStrip, ui.rightDock,
        currentView, isPianoRollVisible, isBottomDockVisible, bottomDockHeight, isLeftSidebarVisible, leftSidebarWidth
    });
}

void ViewManager::toggleViewMode() {
    currentView = (currentView == ViewMode::Arrangement) ? ViewMode::Mixer : ViewMode::Arrangement;
    resized();
}

void ViewManager::openPianoRoll() {
    if (!isPianoRollVisible) {
        isPianoRollVisible = true;
        currentView = ViewMode::Arrangement;
        resized();
    }
}

void ViewManager::closePianoRoll() {
    if (isPianoRollVisible) {
        isPianoRollVisible = false;
        ui.pianoRollUI.setActiveClip(nullptr);
        resized();
    }
}

void ViewManager::showBottomDock(BottomDock::Tab tab) {
    isBottomDockVisible = true;
    ui.bottomDock.showTab(tab);
    resized();
}
