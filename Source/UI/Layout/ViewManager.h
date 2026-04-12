#pragma once
#include <JuceHeader.h>
#include "../UIManager.h"
#include "LayoutHandler.h"

class ViewManager {
public:
    ViewManager(DAWUIComponents& uiComponents, juce::Component& parentComponent);

    void setup();
    void resized();

    // API de control de vistas
    void toggleViewMode();
    void openPianoRoll();
    void closePianoRoll();
    void showBottomDock(BottomDock::Tab tab);

    // Getters de estado (para puentes o lógica externa si es necesario)
    ViewMode getCurrentView() const { return currentView; }
    bool isBottomDockVisibleNow() const { return isBottomDockVisible; }
    bool isLeftSidebarVisibleNow() const { return isLeftSidebarVisible; }
    bool isPianoRollVisibleNow() const { return isPianoRollVisible; }

    // Provisorio: Referencias para compatibilidad con BridgeManager
    bool& getBottomDockVisibleRef() { return isBottomDockVisible; }
    bool& getLeftSidebarVisibleRef() { return isLeftSidebarVisible; }

private:
    void updateLayout();

    DAWUIComponents& ui;
    juce::Component& parent;

    // --- ESTADO INTERNO DEL LAYOUT ---
    ViewMode currentView = ViewMode::Arrangement;
    
    bool isBottomDockVisible = true;
    int bottomDockHeight = 300;
    int lastMixerHeight = 300;
    bool isResizerActive = false;

    bool isLeftSidebarVisible = true;
    int leftSidebarWidth = 200;

    bool isPianoRollVisible = false;
    bool prePianoRollLeftSidebar = true;
    bool prePianoRollBottomDock = true;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ViewManager)
};
