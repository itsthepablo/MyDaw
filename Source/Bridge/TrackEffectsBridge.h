#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Effects/EffectsPanel.h"

class TrackEffectsBridge {
public:
    static void connect(TrackContainer& container,
        EffectsPanel& ui,
        juce::CriticalSection& audioMutex,
        double& sampleRate,
        int& blockSize,
        std::function<void()> onEffectsOpened)
    {
        container.onOpenEffects = [&ui, onEffectsOpened](Track& t) {
            ui.setTrack(&t);
            // Llamamos al callback para que el MainComponent despliegue el Sidebar
            if (onEffectsOpened) onEffectsOpened();
        };
    }
};