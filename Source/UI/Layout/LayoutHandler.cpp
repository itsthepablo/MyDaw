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

    d.hintPanel.setBounds(area.removeFromBottom(28));

    // La TransportBar ha sido unificada en la TopMenuBar

    if (d.isPianoRollVisible) {
        d.pianoRollUI.setVisible(true);
        d.closePianoRollBtn.setVisible(true);

        d.pianoRollUI.setBounds(area);
        d.closePianoRollBtn.setBounds(area.getRight() - 160, area.getY() + 10, 140, 25);

        d.trackContainer.setVisible(false);
        d.playlistUI.setVisible(false);
        d.bottomDock.setVisible(false);
        d.leftSidebar.setVisible(false);
        d.sidebarResizer.setVisible(false);
        d.mixerUI.setVisible(false);
        d.masterChannelUI.setVisible(false);
        d.rightDock.setVisible(false);
    }
    else {
        d.pianoRollUI.setVisible(false);
        d.closePianoRollBtn.setVisible(false);

        if (d.currentView == ViewMode::Mixer) {
            d.masterChannelUI.setVisible(true);

            d.trackContainer.setVisible(false);
            d.playlistUI.setVisible(false);
            d.bottomDock.setVisible(false);
            d.leftSidebar.setVisible(false);
            d.sidebarResizer.setVisible(false);

            d.mixerUI.setVisible(true);

            float scale = area.getHeight() / 600.0f;
            int masterWidth = juce::roundToInt(120.0f * scale);
            d.masterChannelUI.setBounds(area.removeFromLeft(masterWidth));

            d.mixerUI.setBounds(area);
            d.rightDock.setVisible(false);
        }
        else {
            d.masterChannelUI.setVisible(false);
            d.mixerUI.setVisible(false);

            d.trackContainer.setVisible(true);
            d.playlistUI.setVisible(true);

            if (d.isBottomDockVisible) {
                d.bottomDock.setVisible(true);
                
                auto bottomArea = area.removeFromBottom(d.bottomDockHeight);
                
                // El resizer solo se muestra si el Bottom Dock es visible.
                // Sin embargo, su visibilidad real se controla en MainComponent basándose en la pestaña.
                // Aquí solo definimos dónde DEBE ESTAR si es visible.
                d.bottomDockResizer.setBounds(bottomArea.removeFromTop(4)); 
                d.bottomDock.setBounds(bottomArea);
            }
            else {
                d.bottomDock.setVisible(false);
                d.bottomDockResizer.setVisible(false);
            }

            if (d.isLeftSidebarVisible) {
                d.leftSidebar.setVisible(true);
                d.sidebarResizer.setVisible(true);
                d.leftSidebar.setBounds(area.removeFromLeft(d.leftSidebarWidth));
                d.sidebarResizer.setBounds(area.removeFromLeft(4));
            }
            else {
                d.leftSidebar.setVisible(false);
                d.sidebarResizer.setVisible(false);
            }

            // --- RIGHT DOCK (CON INSPECTOR DE CANAL SELECCIONADO) ---
            d.rightDock.setVisible(true);
            int rightDockWidth = d.rightDock.getIdealWidthForHeight(area.getHeight());
            d.rightDock.setBounds(area.removeFromRight(rightDockWidth));

            // --- DISEÑO INTEGRADO: Playlist completa, TrackContainer con espacio para Master ---
            auto trackArea = area.removeFromLeft(250);
            auto masterHeaderArea = trackArea.removeFromBottom(100);
            
            d.masterStrip.setVisible(true);
            d.masterStrip.setBounds(masterHeaderArea);
            
            d.trackContainer.setVisible(true);
            d.trackContainer.setBounds(trackArea);

            d.playlistUI.setVisible(true);
            d.playlistUI.setBounds(area); // La Playlist toma el resto del alto total
        }
    }
}