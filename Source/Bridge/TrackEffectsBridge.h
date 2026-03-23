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
            if (onEffectsOpened) onEffectsOpened();
            };

        container.onActiveTrackChanged = [&ui](Track* t) {
            ui.setTrack(t);
            };

        // --- CORREGIDO: Capturamos el blockSize real del motor de audio ---
        ui.onAddEffect = [&audioMutex, &sampleRate, &blockSize, &ui](Track& t) {
            auto* host = new VSTHost();

            host->loadPluginAsync(sampleRate, [host, &t, &audioMutex, &sampleRate, &blockSize, &ui](bool success) {
                if (success) {
                    juce::MessageManager::callAsync([host, &t, &audioMutex, &sampleRate, &blockSize, &ui]() {
                        juce::ScopedLock sl(audioMutex);

                        // --- LA CLAVE ---
                        // Despertamos al plugin pasandole la configuracion exacta en la que esta corriendo el DAW
                        int currentBlockSize = blockSize > 0 ? blockSize : 512;
                        host->prepareToPlay(sampleRate, currentBlockSize);

                        t.plugins.add(host);
                        EffectsPanel::pluginIsInstrumentMap[(void*)host] = (t.getType() == TrackType::MIDI && t.plugins.size() == 1);
                        ui.updateSlots();
                        });
                }
                else {
                    delete host;
                }
                });
            };

        ui.onOpenEffect = [](Track& t, int idx) {
            if (idx >= 0 && idx < t.plugins.size()) {
                if (auto* plugin = t.plugins[idx]) {
                    plugin->showWindow();
                }
            }
            };

        ui.onBypassChanged = [&audioMutex](Track& t, int idx, bool bypassed) {
            juce::ScopedLock sl(audioMutex);
            if (idx >= 0 && idx < t.plugins.size()) {
                if (auto* plugin = t.plugins[idx]) {
                    plugin->setBypassed(bypassed);
                }
            }
            };

        ui.onReorderEffects = [&audioMutex, &ui](Track& t, int oldIdx, int newIdx) {
            juce::ScopedLock sl(audioMutex);
            if (oldIdx >= 0 && oldIdx < t.plugins.size() && newIdx >= 0 && newIdx < t.plugins.size() && oldIdx != newIdx) {
                t.plugins.move(oldIdx, newIdx);
                juce::MessageManager::callAsync([&ui]() {
                    ui.updateSlots();
                    });
            }
            };
    }
};