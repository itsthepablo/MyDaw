#include "LayoutHandler.h"

void LayoutHandler::performLayout(LayoutDependencies d) {
    auto area = d.area;

    d.topMenuBar.setBounds(area.removeFromTop(30));
    d.hintPanel.setBounds(area.removeFromBottom(28));

    auto topArea = area.removeFromTop(45);
    d.toolbarButtons.setBounds(topArea.removeFromRight(550));

    if (d.resourceMeter != nullptr) {
        d.resourceMeter->setBounds(topArea.removeFromRight(100).reduced(8, 10));
    }
    d.transportBar.setBounds(topArea);

    if (d.isPianoRollVisible) {
        d.pianoRollUI.setVisible(true);
        d.closePianoRollBtn.setVisible(true);

        d.pianoRollUI.setBounds(area);
        d.closePianoRollBtn.setBounds(area.getRight() - 160, area.getY() + 10, 140, 25);

        d.trackContainer.setVisible(false);
        d.playlistUI.setVisible(false);
        d.bottomDock.setVisible(false);
        d.bottomDockResizer.setVisible(false);
        d.leftSidebar.setVisible(false);
        d.sidebarResizer.setVisible(false);
        d.mixerUI.setVisible(false);
        d.masterChannelUI.setVisible(false);
    }
    else {
        d.pianoRollUI.setVisible(false);
        d.closePianoRollBtn.setVisible(false);

        if (d.currentView == ViewMode::Mixer) {
            d.masterChannelUI.setVisible(true); 

            d.trackContainer.setVisible(false);
            d.playlistUI.setVisible(false);
            d.bottomDock.setVisible(false);
            d.bottomDockResizer.setVisible(false);
            d.leftSidebar.setVisible(false);
            d.sidebarResizer.setVisible(false);

            d.mixerUI.setVisible(true);

            float scale = area.getHeight() / 600.0f;
            int masterWidth = juce::roundToInt(120.0f * scale);
            d.masterChannelUI.setBounds(area.removeFromLeft(masterWidth));

            d.mixerUI.setBounds(area);
        }
        else {
            d.masterChannelUI.setVisible(false); 
            d.mixerUI.setVisible(false);

            d.trackContainer.setVisible(true);
            d.playlistUI.setVisible(true);

            if (d.isBottomDockVisible) {
                d.bottomDock.setVisible(true);
                d.bottomDockResizer.setVisible(true);
                d.bottomDock.setBounds(area.removeFromBottom(d.bottomDockHeight));
                d.bottomDockResizer.setBounds(area.removeFromBottom(4));
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

            d.trackContainer.setBounds(area.removeFromLeft(250));
            d.playlistUI.setBounds(area);
        }
    }
}