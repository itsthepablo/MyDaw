#pragma once
#include <JuceHeader.h>
#include "../../../../Tracks/TrackContainer.h"
#include "../EffectsPanel.h"
#include "../../../../Playlist/PlaylistComponent.h"
#include "../../../../Native_Plugins/UtilityPlugin.h" 
#include "../../../../Native_Plugins/Orion/OrionPlugin.h"

#include "../../../../Engine/Core/AudioEngine.h"

class TrackEffectsBridge {
public:
    static void connect(TrackContainer& container,
        EffectsPanel& ui,
        MixerComponent& mixerUI,
        MixerComponent& miniMixerUI,
        MixerChannelUI* masterChannelUI, // NUEVO - El Master Channel complejo
        juce::CriticalSection& audioMutex,
        double& sampleRate,
        int& blockSize,
        AudioEngine& engine, 
        std::function<void()> onEffectsOpened,
        PlaylistComponent* playlist = nullptr)
    {
        // --- HELPER PARA SINCRONIZAR TODAS LAS UIs ---
        auto refreshUIs = [&ui, &mixerUI, &miniMixerUI, masterChannelUI]() {
            ui.updateSlots();
            mixerUI.updateChannels();
            miniMixerUI.updateChannels();
            if (masterChannelUI) masterChannelUI->updateUI();
        };

        container.onOpenEffects = [&ui, onEffectsOpened](Track& t) {
            ui.setTrack(&t);
            if (onEffectsOpened) onEffectsOpened();
        };

        container.onActiveTrackChanged = [&ui, playlist](Track* t) {
            ui.setTrack(t);
            if (playlist) playlist->setSelectedTrack(t);
        };

        // --- LÓGICA COMPARTIDA DE CARGA VST3 ---
        auto addVST3Action = [&audioMutex, &sampleRate, &blockSize, refreshUIs, &engine, &container](Track& t) {
            auto* host = new VSTHost();
            host->loadPluginAsync(sampleRate, [host, &t, &audioMutex, sampleRate, blockSize, refreshUIs, &engine, &container](bool success) {
                if (success) {
                    juce::MessageManager::callAsync([host, &t, &audioMutex, sampleRate, blockSize, refreshUIs, &engine, &container]() {
                        juce::ScopedLock sl(audioMutex);
                        int currentBlockSize = blockSize > 0 ? blockSize : 512;
                        host->prepareToPlay(sampleRate, currentBlockSize);
                        t.plugins.add(host);
                        t.allocatePdcBuffer(); 
                        host->setIsInstrument(t.getType() == TrackType::MIDI && t.plugins.size() == 1);
                        host->onSidechainChanged = [&engine, &container]() { engine.routingMatrix.commitNewTopology(container.getTracks()); };
                        engine.routingMatrix.commitNewTopology(container.getTracks());
                        refreshUIs();
                        
                        // IMPORTANTE: Notificar al motor de audio
                        t.commitSnapshot();
                        
                        host->showWindow(&container);
                    });
                }
            });
        };

        ui.onAddVST3 = addVST3Action;
        mixerUI.onAddVST3 = addVST3Action;
        miniMixerUI.onAddVST3 = addVST3Action;
        if (masterChannelUI) masterChannelUI->onAddVST3 = addVST3Action;

        // --- LÓGICA COMPARTIDA NATIVO ---
        auto addNativeAction = [&audioMutex, &sampleRate, &blockSize, refreshUIs, &engine, &container](Track& t) {
            juce::MessageManager::callAsync([&t, &audioMutex, sampleRate, blockSize, refreshUIs, &engine, &container]() {
                juce::ScopedLock sl(audioMutex);
                auto* utility = new UtilityPlugin();
                int currentBlockSize = blockSize > 0 ? blockSize : 512;
                utility->prepareToPlay(sampleRate, currentBlockSize);
                t.plugins.add(utility);
                t.allocatePdcBuffer(); 
                utility->setIsInstrument(false); 
                utility->onSidechainChanged = [&engine, &container]() { engine.routingMatrix.commitNewTopology(container.getTracks()); };
                engine.routingMatrix.commitNewTopology(container.getTracks());
                refreshUIs();
                
                // IMPORTANTE: Sincronizar motor de audio
                t.commitSnapshot();
                
                utility->showWindow(&container);
            });
        };

        ui.onAddNativeUtility = addNativeAction;
        mixerUI.onAddNativeUtility = addNativeAction;
        miniMixerUI.onAddNativeUtility = addNativeAction;
        if (masterChannelUI) masterChannelUI->onAddNativeUtility = addNativeAction;

        // --- LÓGICA ORION ---
        auto addOrionAction = [&audioMutex, &sampleRate, &blockSize, refreshUIs, &engine, &container](Track& t) {
            juce::MessageManager::callAsync([&t, &audioMutex, sampleRate, blockSize, refreshUIs, &engine, &container]() {
                juce::ScopedLock sl(audioMutex);
                auto* orion = new OrionPlugin();
                int currentBlockSize = blockSize > 0 ? blockSize : 512;
                orion->prepareToPlay(sampleRate, currentBlockSize);
                t.plugins.add(orion);
                t.allocatePdcBuffer();
                orion->setIsInstrument(true);
                orion->onSidechainChanged = [&engine, &container]() { engine.routingMatrix.commitNewTopology(container.getTracks()); };
                engine.routingMatrix.commitNewTopology(container.getTracks());
                refreshUIs();
                
                // IMPORTANTE: Sincronizar motor de audio para que Orion suene
                t.commitSnapshot();
                
                // Orion se muestra integrado, no necesita showWindow()
            });
        };

        ui.onAddNativeOrion = addOrionAction;

        // --- OPEN / BYPASS / DELETE ---
        auto openEffectAction = [&container](Track& t, int idx) {
            if (idx >= 0 && idx < t.plugins.size()) {
                if (auto* plugin = t.plugins[idx]) plugin->showWindow(&container); 
            }
        };
        ui.onOpenEffect = openEffectAction;
        mixerUI.onOpenPlugin = openEffectAction;
        miniMixerUI.onOpenPlugin = openEffectAction;
        if (masterChannelUI) masterChannelUI->onOpenPlugin = openEffectAction;

        auto bypassAction = [&audioMutex, refreshUIs](Track& t, int idx, bool bypassed) {
            juce::ScopedLock sl(audioMutex);
            if (idx >= 0 && idx < t.plugins.size()) {
                if (auto* plugin = t.plugins[idx]) plugin->setBypassed(bypassed);
                juce::MessageManager::callAsync([refreshUIs]() { refreshUIs(); });
            }
        };
        ui.onBypassChanged = bypassAction;
        mixerUI.onBypassChanged = bypassAction;
        miniMixerUI.onBypassChanged = bypassAction;
        if (masterChannelUI) masterChannelUI->onBypassChanged = bypassAction;

        auto deleteEffectAction = [&audioMutex, refreshUIs, &engine, &container](Track& t, int idx) {
            juce::ScopedLock sl(audioMutex);
            if (idx >= 0 && idx < t.plugins.size()) {
                t.plugins.remove(idx); 
                engine.routingMatrix.commitNewTopology(container.getTracks());
                
                // IMPORTANTE: Sincronizar tras eliminación
                t.commitSnapshot();
                
                juce::MessageManager::callAsync([refreshUIs]() { refreshUIs(); });
            }
        };
        ui.onDeleteEffect = deleteEffectAction;
        mixerUI.onDeleteEffect = deleteEffectAction;
        miniMixerUI.onDeleteEffect = deleteEffectAction;
        if (masterChannelUI) masterChannelUI->onDeleteEffect = deleteEffectAction;

        // --- ENVÍOS ---
        auto addSendAction = [&engine, &container, refreshUIs](Track& t) {
            t.routingData.sends.push_back({ -1, 1.0f, false, false }); 
            engine.routingMatrix.commitNewTopology(container.getTracks());
            refreshUIs();
        };
        ui.onAddSend = addSendAction;
        mixerUI.onAddSend = addSendAction;
        miniMixerUI.onAddSend = addSendAction;
        if (masterChannelUI) masterChannelUI->onAddSend = addSendAction;

        auto deleteSendAction = [&engine, &container, refreshUIs](Track& t, int idx) {
            if (idx >= 0 && idx < (int)t.routingData.sends.size()) {
                t.routingData.sends.erase(t.routingData.sends.begin() + idx);
                engine.routingMatrix.commitNewTopology(container.getTracks());
                refreshUIs();
            }
        };
        ui.onDeleteSend = deleteSendAction;
        mixerUI.onDeleteSend = deleteSendAction;
        miniMixerUI.onDeleteSend = deleteSendAction;
        if (masterChannelUI) masterChannelUI->onDeleteSend = deleteSendAction;

        auto createAutoAction = [&container](Track& t, int paramId, juce::String paramName) {
            container.createAutomation(t.getId(), paramId, paramName);
        };
        mixerUI.onCreateAutomation = createAutoAction;
        miniMixerUI.onCreateAutomation = createAutoAction;
        if (masterChannelUI) masterChannelUI->onCreateAutomation = createAutoAction;

        ui.onChangeSend = [&engine, &container](Track& t) {
            engine.routingMatrix.commitNewTopology(container.getTracks());
        };

        ui.getAvailableTracks = [&container]() {
            juce::Array<Track*> trackList;
            for (auto* t : container.getTracks()) trackList.add(t);
            return trackList;
        };
    }
};