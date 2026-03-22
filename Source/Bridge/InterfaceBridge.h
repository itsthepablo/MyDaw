#pragma once
#include <JuceHeader.h>
#include "../UI/Buttons/ToolbarButtons.h"
#include "../Mixer/MixerComponent.h"
#include "../Effects/EffectsPanel.h"
#include "../UI/PickerPanel.h" // NUEVO
#include "../Tracks/TrackContainer.h"

class InterfaceBridge {
public:
    static void connect(ToolbarButtons& toolbar,
        bool& isMixerVisible, bool& isEffectsPanelVisible, bool& isPickerVisible, // NUEVO BOOLEANO
        MixerComponent& mixerUI, EffectsPanel& effectsPanelUI, PickerPanel& pickerPanelUI, // NUEVA UI
        TrackContainer& trackContainer,
        std::function<void()> triggerResize)
    {
        // NUEVO EVENTO
        toolbar.onTogglePicker = [&isPickerVisible, triggerResize] {
            isPickerVisible = !isPickerVisible;
            triggerResize();
        };

        toolbar.onToggleMixer = [&isMixerVisible, triggerResize] {
            isMixerVisible = !isMixerVisible;
            triggerResize();
        };

        toolbar.onToggleFx = [&isEffectsPanelVisible, triggerResize, &effectsPanelUI, &trackContainer] {
            isEffectsPanelVisible = !isEffectsPanelVisible;
            
            if (isEffectsPanelVisible) {
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