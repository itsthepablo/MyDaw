#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Instruments/InstrumentPanel.h"
#include "../Effects/EffectsPanel.h"
#include "../UI/BottomDock.h"
#include "../PluginHost/VSTHost.h"
#include "../Engine/Core/AudioEngine.h" // <-- Para conocer los valores en tiempo real

class TrackInstrumentBridge {
public:
    static void connect(TrackContainer& trackContainer,
        InstrumentPanel& instrumentPanel,
        EffectsPanel& effectsPanel,
        BottomDock& bottomDock,
        bool& isBottomDockVisible,
        juce::CriticalSection& audioMutex,
        AudioEngine& audioEngine, // <-- AHORA PASAMOS LA REFERENCIA AL MOTOR ENTERO
        std::function<void()> triggerResize)
    {
        trackContainer.onOpenInstrument = [&isBottomDockVisible, &bottomDock, &instrumentPanel, triggerResize](Track& t) {
            isBottomDockVisible = true;
            bottomDock.showTab(BottomDock::InstrumentTab);
            instrumentPanel.setTrack(&t);
            triggerResize();
            };

        instrumentPanel.onAddInstrument = [&audioMutex, &audioEngine, &trackContainer, &effectsPanel, &instrumentPanel, &bottomDock](Track& t) {
            auto* newPlugin = new VSTHost();

            // Leemos el sample rate y block size REALES justo en el momento de cargar
            double currentSampleRate = audioEngine.clock.sampleRate > 0.0 ? audioEngine.clock.sampleRate : 44100.0;
            int currentBlockSize = audioEngine.clock.maxBlockSize > 0 ? audioEngine.clock.maxBlockSize : 512;

            newPlugin->loadPluginAsync(currentSampleRate, [&t, newPlugin, &audioMutex, currentSampleRate, currentBlockSize, &trackContainer, &effectsPanel, &instrumentPanel, &bottomDock](bool success) {
                if (success) {
                    {
                        const juce::ScopedLock sl(audioMutex);
                        t.plugins.add(newPlugin);
                        t.addPluginName(newPlugin->getLoadedPluginName());
                        EffectsPanel::pluginIsInstrumentMap[(void*)newPlugin] = true;

                        // DESPERTAR AL PLUGIN CON LOS DATOS REALES DE LA TARJETA DE SONIDO
                        newPlugin->prepareToPlay(currentSampleRate, currentBlockSize);
                    }

                    juce::MessageManager::callAsync([&t, newPlugin, &trackContainer, &effectsPanel, &instrumentPanel, &bottomDock]() {
                        trackContainer.refreshTrackPanels();

                        if (bottomDock.getCurrentTab() == BottomDock::EffectsTab)
                            effectsPanel.updateSlots();

                        if (bottomDock.getCurrentTab() == BottomDock::InstrumentTab)
                            instrumentPanel.updateInstrumentView();

                        newPlugin->showWindow();
                        });
                }
                else {
                    delete newPlugin;
                }
                });
            };

        instrumentPanel.onOpenInstrumentWindow = [](Track& t, BaseEffect* effect) {
            if (effect != nullptr) {
                effect->showWindow();
            }
            };
    }
};