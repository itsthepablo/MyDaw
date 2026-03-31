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
        
        // initialPendingCounts[i] = cuantos tracks faltan para que track[i] pueda empezar (incluye hijos y sidechains)
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
        
        // --- SIDECHAIN DEPENDENCIES (Zero Latency) ---
        std::map<int, int> idToIndex;
        for (int i = 0; i < n; ++i) idToIndex[tracks[i]->getId()] = i;

        for (int i = 0; i < n; ++i) {
            auto sidechainDeps = tracks[i]->getSidechainDependencies();
            for (int sourceId : sidechainDeps) {
                if (idToIndex.count(sourceId)) {
                    int sourceIdx = idToIndex[sourceId];
                    if (sourceIdx != i) { 
                        // El track i debe esperar al track sourceIdx (sidechain)
                        newState->notifyList[sourceIdx].push_back(i);
                        newState->initialPendingCounts[i]++;
                    }
                }
            }
        }

        auto* oldState = currentState.exchange(newState);
        if (oldState) {
            juce::MessageManager::callAsync([oldState]() { delete oldState; }); // Lock-free cleanup
        }
    }

    // Llamada desde el Hilo de Audio. Estrictamente Wait-Free.
    TopoState* getAudioThreadState() const noexcept {
        return currentState.load(std::memory_order_acquire);
    }

    ~RoutingMatrix() {
        auto* old = currentState.exchange(nullptr);
        if (old) delete old;
    }
};
