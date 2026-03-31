#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include <map>
#include "../../Tracks/Track.h"

class RoutingMatrix {
public:
    struct TopoState {
        juce::Array<Track*> activeTracks;
        std::vector<int> parentIndices;

        // notifyList[i] = lista de tracks que dependen de track[i] y deben ser notificados al terminar.
        std::vector<std::vector<int>> notifyList;

        // childrenOf[i] = indices de los tracks que son hijos (en carpeta) de track[i] (para suma de audio)
        std::vector<std::vector<int>> childrenOf;
        
        // initialPendingCounts[i] = cuantos tracks faltan para que track[i] pueda empezar (hijos, sidechains, sends)
        std::vector<int> initialPendingCounts;
    };

    // Puntero atomico hacia la topologia activa real
    std::atomic<TopoState*> currentState { nullptr };

    // Llamada desde el Hilo de la UI (Message Thread) cuando se altera la lista de pistas.
    void commitNewTopology(const juce::OwnedArray<Track>& tracks) {
        auto* newState = new TopoState();
        newState->activeTracks.addArray(tracks);

        int n = tracks.size();

        // --- parentIndices (Estructura de Carpetas para Suma) ---
        newState->parentIndices.resize(n, -1);
        std::vector<int> parentStack;
        for (int i = 0; i < n; ++i) {
            if (!parentStack.empty()) newState->parentIndices[i] = parentStack.back();
            if (tracks[i]->folderDepthChange == 1) parentStack.push_back(i);
            else if (tracks[i]->folderDepthChange < 0) {
                int drops = -tracks[i]->folderDepthChange;
                for (int d = 0; d < drops; ++d) { if (!parentStack.empty()) parentStack.pop_back(); }
            }
        }

        // --- childrenOf + initialPendingCounts + notifyList ---
        newState->childrenOf.resize(n);
        newState->notifyList.resize(n);
        newState->initialPendingCounts.resize(n, 0);

        for (int i = 0; i < n; ++i) {
            int parent = newState->parentIndices[i];
            if (parent >= 0 && parent < n) {
                // Relación de Carpeta: El padre espera a i para sumarlo.
                newState->childrenOf[parent].push_back(i);
                newState->notifyList[i].push_back(parent);
                newState->initialPendingCounts[parent]++;
            }
        }
        
        // Mapeo rápido de ID a Índice
        std::map<int, int> idToIndex;
        for (int i = 0; i < n; ++i) idToIndex[tracks[i]->getId()] = i;

        // --- SIDECHAIN DEPENDENCIES ---
        for (int i = 0; i < n; ++i) {
            auto sidechainDeps = tracks[i]->getSidechainDependencies();
            for (int sourceId : sidechainDeps) {
                if (idToIndex.count(sourceId)) {
                    int sourceIdx = idToIndex[sourceId];
                    if (sourceIdx != i) { 
                        newState->notifyList[sourceIdx].push_back(i);
                        newState->initialPendingCounts[i]++;
                    }
                }
            }
        }

        // --- SEND DEPENDENCIES (ENVÍOS) ---
        for (int i = 0; i < n; ++i) {
            for (const auto& send : tracks[i]->sends) {
                if (!send.isMuted && idToIndex.count(send.targetTrackId)) {
                    int targetIdx = idToIndex[send.targetTrackId];
                    // El destino (target) debe esperar al origen (i)
                    if (targetIdx != i) {
                        newState->notifyList[i].push_back(targetIdx);
                        newState->initialPendingCounts[targetIdx]++;
                    }
                }
            }
        }

        auto* oldState = currentState.exchange(newState);
        if (oldState) {
            juce::MessageManager::callAsync([oldState]() { delete oldState; }); 
        }
    }

    TopoState* getAudioThreadState() const noexcept {
        return currentState.load(std::memory_order_acquire);
    }

    ~RoutingMatrix() {
        auto* old = currentState.exchange(nullptr);
        if (old) delete old;
    }
};
