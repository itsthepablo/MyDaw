#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Effects/EffectsPanel.h"
#include "../Native_Plugins/UtilityPlugin.h" 

#include "../Engine/Core/AudioEngine.h"

class TrackEffectsBridge {
public:
    static void connect(TrackContainer& container,
        EffectsPanel& ui,
        juce::CriticalSection& audioMutex,
        double& sampleRate,
        int& blockSize,
        AudioEngine& engine, 
        std::function<void()> onEffectsOpened)
    {
        container.onOpenEffects = [&ui, onEffectsOpened](Track& t) {
            ui.setTrack(&t);
            if (onEffectsOpened) onEffectsOpened();
        };

        container.onActiveTrackChanged = [&ui](Track* t) {
            ui.setTrack(t);
        };

        // --- CARGA VST3 ---
        ui.onAddVST3 = [&audioMutex, &sampleRate, &blockSize, &ui, &engine, &container](Track& t) {
            auto* host = new VSTHost();
            
            host->loadPluginAsync(sampleRate, [host, &t, &audioMutex, sampleRate, blockSize, &ui, &engine, &container](bool success) {
                if (success) {
                    juce::MessageManager::callAsync([host, &t, &audioMutex, sampleRate, blockSize, &ui, &engine, &container]() {
                        juce::ScopedLock sl(audioMutex);
                        int currentBlockSize = blockSize > 0 ? blockSize : 512;
                        host->prepareToPlay(sampleRate, currentBlockSize);
                        t.plugins.add(host);
                        t.allocatePdcBuffer(); 
                        host->setIsInstrument(t.getType() == TrackType::MIDI && t.plugins.size() == 1);
                        
                        host->onSidechainChanged = [&engine, &container]() {
                            engine.routingMatrix.commitNewTopology(container.getTracks());
                        };

                        engine.routingMatrix.commitNewTopology(container.getTracks());
                        ui.updateSlots();
                        host->showWindow(&container);
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
                        
                        if (base64State.isNotEmpty()) {
                            juce::MemoryBlock mb;
                            mb.fromBase64Encoding(base64State);
                            host->setStateInformation(mb.getData(), (int)mb.getSize());
                        }

                        host->sidechainSourceTrackId.store(sidechainSourceId);

                        t.plugins.add(host);
                        t.allocatePdcBuffer();
                        host->setIsInstrument(t.getType() == TrackType::MIDI && t.plugins.size() == 1);
                        
                        host->onSidechainChanged = [&engine, &container]() {
                            engine.routingMatrix.commitNewTopology(container.getTracks());
                        };

                        engine.routingMatrix.commitNewTopology(container.getTracks());
                        ui.updateSlots();
                        host->showWindow(&container);
                    });
                } else {
                    delete host; 
                }
            });
        };

        // --- CARGA NATIVO ---
        ui.onAddNativeUtility = [&audioMutex, &sampleRate, &blockSize, &ui, &engine, &container](Track& t) {
            juce::MessageManager::callAsync([&t, &audioMutex, sampleRate, blockSize, &ui, &engine, &container]() {
                juce::ScopedLock sl(audioMutex);
                auto* utility = new UtilityPlugin();
                int currentBlockSize = blockSize > 0 ? blockSize : 512;
                utility->prepareToPlay(sampleRate, currentBlockSize);
                t.plugins.add(utility);
                t.allocatePdcBuffer(); 
                utility->setIsInstrument(false); 
                utility->onSidechainChanged = [&engine, &container]() {
                    engine.routingMatrix.commitNewTopology(container.getTracks());
                };
                engine.routingMatrix.commitNewTopology(container.getTracks());
                ui.updateSlots();
                utility->showWindow(&container);
            });
        };

        // --- NUEVO: GESTIÓN DE ENVÍOS (SENDS) ---
        ui.onAddSend = [&engine, &container, &ui](Track& t) {
            t.sends.push_back({ -1, 1.0f, false, false }); 
            engine.routingMatrix.commitNewTopology(container.getTracks());
            ui.updateSlots();
        };

        ui.onDeleteSend = [&engine, &container, &ui](Track& t, int idx) {
            if (idx >= 0 && idx < (int)t.sends.size()) {
                t.sends.erase(t.sends.begin() + idx);
                engine.routingMatrix.commitNewTopology(container.getTracks());
                ui.updateSlots();
            }
        };

        ui.onChangeSend = [&engine, &container](Track& t) {
            engine.routingMatrix.commitNewTopology(container.getTracks());
        };

        ui.getAvailableTracks = [&container]() {
            juce::Array<Track*> trackList;
            for (auto* t : container.getTracks())
                trackList.add(t);
            return trackList;
        };

        ui.onOpenEffect = [&container](Track& t, int idx) {
            if (idx >= 0 && idx < t.plugins.size()) {
                if (auto* plugin = t.plugins[idx]) {
                    plugin->showWindow(&container); 
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