#pragma once
#include <JuceHeader.h>
#include "../UI/Buttons/ToolbarButtons.h"
#include "../UI/TopMenuBar/TopMenuBar.h"
#include "../UI/BottomDock.h"
#include "../UI/Panels/Effects/EffectsPanel.h"
#include "../UI/LeftSidebar.h"
#include "../Tracks/TrackContainer.h"

class InterfaceBridge {
public:
    static void connect(TopMenuBar& topMenu,
        bool& isBottomDockVisible, bool& isLeftSidebarVisible,
        BottomDock& bottomDock, EffectsPanel& effectsPanelUI, LeftSidebar& leftSidebar,
        TrackContainer& trackContainer,
        std::function<void()> triggerResize,
        std::function<void()> toggleMixerMode)
    {
        topMenu.onTogglePicker = [&isLeftSidebarVisible, &leftSidebar, triggerResize] {
            if (!isLeftSidebarVisible) {
                isLeftSidebarVisible = true; leftSidebar.showTab(LeftSidebar::PickerTab);
            }
            else {
                if (leftSidebar.getCurrentTab() == LeftSidebar::PickerTab) isLeftSidebarVisible = false;
                else leftSidebar.showTab(LeftSidebar::PickerTab);
            }
            triggerResize();
            };

        topMenu.onToggleFiles = [&isLeftSidebarVisible, &leftSidebar, triggerResize] {
            if (!isLeftSidebarVisible) {
                isLeftSidebarVisible = true; leftSidebar.showTab(LeftSidebar::FilesTab);
            }
            else {
                if (leftSidebar.getCurrentTab() == LeftSidebar::FilesTab) isLeftSidebarVisible = false;
                else leftSidebar.showTab(LeftSidebar::FilesTab);
            }
            triggerResize();
            };

        topMenu.onToggleFx = [&isBottomDockVisible, &bottomDock, triggerResize, &effectsPanelUI, &trackContainer] {
            if (!isBottomDockVisible) {
                isBottomDockVisible = true; bottomDock.showTab(BottomDock::EffectsTab);
            }
            else {
                if (bottomDock.getCurrentTab() == BottomDock::EffectsTab) isBottomDockVisible = false;
                else bottomDock.showTab(BottomDock::EffectsTab);
            }
            if (isBottomDockVisible && bottomDock.getCurrentTab() == BottomDock::EffectsTab) {
                bool found = false;
                for (auto* t : trackContainer.getTracks()) {
                    if (t->getType() == TrackType::Audio || t->getType() == TrackType::MIDI) {
                        effectsPanelUI.setTrack(t); found = true; break;
                    }
                }
                if (!found) effectsPanelUI.setTrack(nullptr);
            }
            triggerResize();
            };

        topMenu.onToggleMixer = [&isBottomDockVisible, &bottomDock, triggerResize] {
            if (!isBottomDockVisible) {
                isBottomDockVisible = true;
                bottomDock.showTab(BottomDock::MixerTab);
            }
            else {
                if (bottomDock.getCurrentTab() == BottomDock::MixerTab)
                    isBottomDockVisible = false;
                else
                    bottomDock.showTab(BottomDock::MixerTab);
            }
            triggerResize();
        };

        topMenu.onToggleRack = [&isBottomDockVisible, &bottomDock, triggerResize] {
            if (!isBottomDockVisible) {
                isBottomDockVisible = true; bottomDock.showTab(BottomDock::RackTab);
            }
            else {
                if (bottomDock.getCurrentTab() == BottomDock::RackTab) isBottomDockVisible = false;
                else bottomDock.showTab(BottomDock::RackTab);
            }
            triggerResize();
            };
    }
};