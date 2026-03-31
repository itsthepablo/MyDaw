#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Effects/EffectsPanel.h"
#include "../Native_Plugins/UtilityPlugin.h" // <-- Inclusión vital

#include "../Engine/Core/AudioEngine.h"

class TrackEffectsBridge {
public:
    static void connect(TrackContainer& container,
        EffectsPanel& ui,
        juce::CriticalSection& audioMutex,
        double& sampleRate,
        int& blockSize,
        AudioEngine& engine, // NUEVO
        std::function<void()> onEffectsOpened)
    {
        container.onOpenEffects = [&ui, onEffectsOpened](Track& t) {
            ui.setTrack(&t);
            if (onEffectsOpened) onEffectsOpened();
        };

        container.onActiveTrackChanged = [&ui](Track* t) {
            ui.setTrack(t);
        };

        // --- SEÑAL 1: Carga VST3 (Anterior onAddEffect) ---
        ui.onAddVST3 = [&audioMutex, &sampleRate, &blockSize, &ui, &engine, &container](Track& t) {
            auto* host = new VSTHost();
            
            host->loadPluginAsync(sampleRate, [host, &t, &audioMutex, sampleRate, blockSize, &ui, &engine, &container](bool success) {
                if (success) {
                    juce::MessageManager::callAsync([host, &t, &audioMutex, sampleRate, blockSize, &ui, &engine, &container]() {
                        juce::ScopedLock sl(audioMutex);
                        int currentBlockSize = blockSize > 0 ? blockSize : 512;
                        host->prepareToPlay(sampleRate, currentBlockSize);
                        t.plugins.add(host);
                        t.allocatePdcBuffer(); // RAM: alocar PDC buffer ahora que hay un plugin
                        host->setIsInstrument(t.getType() == TrackType::MIDI && t.plugins.size() == 1);
                        engine.routingMatrix.commitNewTopology(container.getTracks());
                        ui.updateSlots();
                    });
                }
            });
        };

        ui.onAddVST3FromFile = [&audioMutex, &sampleRate, &blockSize, &ui, &engine, &container](Track& t, const juce::String& path, const juce::String& base64State, int sidechainSourceId) {
            auto* host = new VSTHost();
            host->loadPluginFromPath(path, sampleRate, [host, &t, &audioMutex, sampleRate, blockSize, &ui, &engine, &container, base64State, sidechainSourceId](bool success) {
                if (success) {
                    juce::MessageManager::callAsync([host, &t, &audioMutex, sampleRate, blockSize, &ui, &engine, &container, base64State, sidechainSourceId]() {
                        juce::ScopedLock sl(audioMutex);
                        int currentBlockSize = blockSize > 0 ? blockSize : 512;
                        host->prepareToPlay(sampleRate, currentBlockSize);
                        
                        // Restaurar el preset (estado binario)
                        if (base64State.isNotEmpty()) {
                            juce::MemoryBlock mb;
                            mb.fromBase64Encoding(base64State);
                            host->setStateInformation(mb.getData(), (int)mb.getSize());
                        }

                        // Restaurar sidechain
                        host->sidechainSourceTrackId.store(sidechainSourceId);

                        t.plugins.add(host);
                        t.allocatePdcBuffer();
                        host->setIsInstrument(t.getType() == TrackType::MIDI && t.plugins.size() == 1);
                        engine.routingMatrix.commitNewTopology(container.getTracks());
                        ui.updateSlots();
                    });
                } else {
                    delete host; 
                }
            });
        };

        // --- SEÑAL 2: Carga Plugin Nativo ---
        ui.onAddNativeUtility = [&audioMutex, &sampleRate, &blockSize, &ui, &engine, &container](Track& t) {
            juce::MessageManager::callAsync([&t, &audioMutex, sampleRate, blockSize, &ui, &engine, &container]() {
                juce::ScopedLock sl(audioMutex);
                
                // Instanciamos directamente nuestra clase nativa
                auto* utility = new UtilityPlugin();
                int currentBlockSize = blockSize > 0 ? blockSize : 512;
                utility->prepareToPlay(sampleRate, currentBlockSize);
                
                t.plugins.add(utility);
                t.allocatePdcBuffer(); // RAM: alocar PDC buffer ahora que hay un plugin
                utility->setIsInstrument(false); // Utility siempre es un efecto
                engine.routingMatrix.commitNewTopology(container.getTracks());
                ui.updateSlots();
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

        ui.onReorderEffects = [&audioMutex, &ui, &engine, &container](Track& t, int oldIdx, int newIdx) {
            juce::ScopedLock sl(audioMutex);
            if (oldIdx == -1 && newIdx == -1) {
                // Caso especial: cambio de Sidechain (o refresco de ruteo)
                engine.routingMatrix.commitNewTopology(container.getTracks());
                return;
            }
            if (oldIdx >= 0 && oldIdx < t.plugins.size() && newIdx >= 0 && newIdx < t.plugins.size() && oldIdx != newIdx) {
                t.plugins.move(oldIdx, newIdx);
                engine.routingMatrix.commitNewTopology(container.getTracks());
                juce::MessageManager::callAsync([&ui]() {
                    ui.updateSlots();
                });
            }
        };

        ui.onDeleteEffect = [&audioMutex, &ui, &engine, &container](Track& t, int idx) {
            juce::ScopedLock sl(audioMutex);
            if (idx >= 0 && idx < t.plugins.size()) {
                t.plugins.remove(idx); 
                engine.routingMatrix.commitNewTopology(container.getTracks());
                juce::MessageManager::callAsync([&ui]() {
                    ui.updateSlots();
                });
            }
        };
    }
};