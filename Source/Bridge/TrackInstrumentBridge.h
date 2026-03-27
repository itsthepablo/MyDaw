#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Instruments/InstrumentPanel.h"
#include "../Effects/EffectsPanel.h"
#include "../UI/BottomDock.h"
#include "../PluginHost/VSTHost.h"

class TrackInstrumentBridge {
public:
    static void connect(TrackContainer& trackContainer,
                        InstrumentPanel& instrumentPanel,
                        EffectsPanel& effectsPanel,
                        BottomDock& bottomDock,
                        bool& isBottomDockVisible,
                        juce::CriticalSection& audioMutex,
                        double sampleRate,
                        std::function<void()> triggerResize)
    {
        // 1. Mostrar el panel al hacer clic en el track
        trackContainer.onOpenInstrument = [&isBottomDockVisible, &bottomDock, &instrumentPanel, triggerResize](Track& t) {
            isBottomDockVisible = true;
            bottomDock.showTab(BottomDock::InstrumentTab);
            instrumentPanel.setTrack(&t);
            triggerResize();
        };

        // 2. Cargar el VST3 y asignarlo como instrumento
        instrumentPanel.onAddInstrument = [&audioMutex, sampleRate, &trackContainer, &effectsPanel, &instrumentPanel, &bottomDock](Track& t) {
            auto* newPlugin = new VSTHost();
            
            newPlugin->loadPluginAsync(sampleRate, [&t, newPlugin, &audioMutex, &trackContainer, &effectsPanel, &instrumentPanel, &bottomDock](bool success) {
                if (success) {
                    {
                        const juce::ScopedLock sl(audioMutex);
                        t.plugins.add(newPlugin);
                        t.addPluginName(newPlugin->getLoadedPluginName());
                        // Marcamos este plugin en el mapa global como un instrumento
                        EffectsPanel::pluginIsInstrumentMap[(void*)newPlugin] = true;
                    }
                    
                    juce::MessageManager::callAsync([&t, newPlugin, &trackContainer, &effectsPanel, &instrumentPanel, &bottomDock]() {
                        trackContainer.refreshTrackPanels();
                        
                        if (bottomDock.getCurrentTab() == BottomDock::EffectsTab)
                            effectsPanel.updateSlots();
                            
                        if (bottomDock.getCurrentTab() == BottomDock::InstrumentTab)
                            instrumentPanel.updateInstrumentView();
                            
                        newPlugin->showWindow();
                    });
                } else {
                    delete newPlugin; // Liberar si se canceló
                }
            });
        };

        // 3. Reabrir ventana del instrumento
        instrumentPanel.onOpenInstrumentWindow = [](Track& t, BaseEffect* effect) {
            if (effect != nullptr) {
                effect->showWindow();
            }
        };
    }
};