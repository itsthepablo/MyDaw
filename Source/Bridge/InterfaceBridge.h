#pragma once
#include <JuceHeader.h>
#include "../UI/Buttons/ToolbarButtons.h"
#include "../Effects/EffectsPanel.h"
#include "../Tracks/TrackContainer.h"

class InterfaceBridge {
public:
    static void connect(ToolbarButtons& buttons,
        bool& mixerVisible,
        bool& effectsVisible,
        juce::Component& mixer,
        EffectsPanel& effectsPanel, // Usamos la clase específica
        TrackContainer& container,  // Agregado para acceder a los tracks
        std::function<void()> triggerResize)
    {
        // Botón del Mixer
        buttons.showMixerBtn.onClick = [&mixerVisible, &mixer, triggerResize] {
            mixerVisible = !mixerVisible;
            mixer.setVisible(mixerVisible);
            triggerResize();
            };

        // Botón de Efectos (Corregido)
        buttons.showEffectsBtn.onClick = [&effectsVisible, &effectsPanel, &container, triggerResize] {
            effectsVisible = !effectsVisible;

            // Si estamos abriendo el panel y no tiene ninguna pista asignada,
            // cargamos la primera pista por defecto para que no aparezca vacío.
            if (effectsVisible && effectsPanel.getActiveTrack() == nullptr) {
                if (container.getTracks().size() > 0) {
                    effectsPanel.setTrack(container.getTracks()[0]);
                }
            }

            effectsPanel.setVisible(effectsVisible);
            triggerResize();
            };
    }
};