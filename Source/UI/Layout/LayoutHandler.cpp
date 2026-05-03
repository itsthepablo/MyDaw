#include "LayoutHandler.h"

void LayoutHandler::performLayout(LayoutDependencies d) {
    auto area = d.area;
    d.masterStrip.setVisible(false);

    auto topMenuArea = area.removeFromTop(96); // Unificación de TopMenuBar + TransportBar (Diseño 96px)
    d.topMenuBar.setBounds(topMenuArea);

    // --- CORRECCIN: MEDIDOR DESPUS DE LOS EFECTOS ---
    if (d.resourceMeter != nullptr) {
        // El botón de FX termina en X = 1237. Lo colocamos en X = 1250 con un margen de 240px de ancho.
        d.resourceMeter->setBounds(1250, topMenuArea.getY() + 7, 240, 32);
        d.resourceMeter->toFront(false); // Evita que la barra lo tape
    }

    d.hintPanel.setBounds(area.removeFromBottom(34));

    // La TransportBar ha sido unificada en la TopMenuBar

    if (d.currentView == ViewMode::Mixer && !d.isPianoRollVisible) {
        d.masterChannelUI.setVisible(true);
        d.trackContainer.setVisible(false);
        d.playlistUI.setVisible(false);
        d.bottomDock.setVisible(false);
        d.leftSidebar.setVisible(false);
        d.sidebarResizer.setVisible(false);

        d.mixerUI.setVisible(true);
        d.pianoRollUI.setVisible(false);
        d.closePianoRollBtn.setVisible(false);
        if (d.tilingLayout != nullptr) d.tilingLayout->setVisible(false);

        int masterWidth = 200; // Fijo para diseño 1:1
        d.masterChannelUI.setBounds(area.removeFromLeft(masterWidth));
        d.mixerUI.setBounds(area);
        d.rightDock.setVisible(false);
    }
    else {
        // --- VISTA ARRANGEMENT O PIANO ROLL INTEGRADO ---
        d.masterChannelUI.setVisible(false);
        d.mixerUI.setVisible(false);

        // --- SISTEMA DE MOSAICO (Dueño absoluto de la pantalla) ---
        if (d.tilingLayout != nullptr) {
            d.tilingLayout->setVisible(true);
            d.tilingLayout->setBounds(area);
        }

        // ESCONDEMOS SOLO LOS DOCKS FIJOS QUE NO QUEREMOS VER FUERA DEL MOSAICO
        d.leftSidebar.setVisible(false);
        d.sidebarResizer.setVisible(false);
        d.bottomDock.setVisible(false);
        d.bottomDockResizer.setVisible(false);
        d.rightDock.setVisible(false);
        
        // NO APAGAMOS LA PLAYLIST NI EL TRACKCONTAINER (Son usados por el mosaico)
        d.pianoRollUI.setVisible(false);
        d.closePianoRollBtn.setVisible(false);
    }
}