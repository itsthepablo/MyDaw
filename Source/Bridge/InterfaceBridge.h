#pragma once
#include <JuceHeader.h>
#include "../UI/Buttons/ToolbarButtons.h"
#include "../Mixer/MixerComponent.h"
#include "../Effects/EffectsPanel.h"
#include "../UI/LeftSidebar.h"
#include "../Tracks/TrackContainer.h"

class InterfaceBridge {
public:
    static void connect(ToolbarButtons& toolbar,
        bool& isMixerVisible, bool& isLeftSidebarVisible,
        MixerComponent& mixerUI, EffectsPanel& effectsPanelUI, LeftSidebar& leftSidebar,
        TrackContainer& trackContainer,
        std::function<void()> triggerResize)
    {
        toolbar.onTogglePicker = [&isLeftSidebarVisible, &leftSidebar, triggerResize] {
            if (!isLeftSidebarVisible) {
                isLeftSidebarVisible = true;
                leftSidebar.showTab(LeftSidebar::PickerTab);
            } else {
                if (leftSidebar.getCurrentTab() == LeftSidebar::PickerTab) {
                    isLeftSidebarVisible = false; // Ocultar si ya estaba activo
                } else {
                    leftSidebar.showTab(LeftSidebar::PickerTab); // Cambiar de pestaña
                }
            }
            triggerResize();
        };

        toolbar.onToggleMixer = [&isMixerVisible, triggerResize] {
            isMixerVisible = !isMixerVisible;
            triggerResize();
        };

        toolbar.onToggleFx = [&isLeftSidebarVisible, &leftSidebar, triggerResize, &effectsPanelUI, &trackContainer] {
            if (!isLeftSidebarVisible) {
                isLeftSidebarVisible = true;
                leftSidebar.showTab(LeftSidebar::FxTab);
            } else {
                if (leftSidebar.getCurrentTab() == LeftSidebar::FxTab) {
                    isLeftSidebarVisible = false;
                } else {
                    leftSidebar.showTab(LeftSidebar::FxTab);
                }
            }
            
            // Si se acaba de mostrar el FX, buscamos la primera pista válida para mostrar algo
            if (isLeftSidebarVisible && leftSidebar.getCurrentTab() == LeftSidebar::FxTab) {
                bool found = false;
                for (auto* t : trackContainer.getTracks()) {
                    if (t->getType() == TrackType::Audio || t->getType() == TrackType::MIDI) {
                        effectsPanelUI.setTrack(t);
                        found = true;
                        break;
                    }
                }
                if (!found) effectsPanelUI.setTrack(nullptr);
            }
            triggerResize();
        };
    }
};