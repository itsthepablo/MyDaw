#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <vector>
#include "../../Tracks/Track.h"

class RoutingMatrix {
public:
    struct TopoState {
        juce::Array<Track*> activeTracks;
        std::vector<int> parentIndices;

        // Para el Task Graph paralelo:
        // childrenOf[i]  = indices de los tracks que son hijos directos de track[i]
        // initialPendingCounts[i] = numero de hijos que debe esperar track[i] (0 = hoja, lista de inmediato)
        std::vector<std::vector<int>> childrenOf;
        std::vector<int> initialPendingCounts;
    };

    // Puntero atomico hacia la topologia activa real
    std::atomic<TopoState*> currentState { nullptr };

    // Llamada desde el Hilo de la UI (Message Thread) cuando se altera la lista de pistas.
    void commitNewTopology(const juce::OwnedArray<Track>& tracks) {
        auto* newState = new TopoState();
        newState->activeTracks.addArray(tracks);

        int n = tracks.size();

        // --- parentIndices ---
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

        // --- childrenOf + initialPendingCounts (construidos de parentIndices) ---
        newState->childrenOf.resize(n);
        newState->initialPendingCounts.resize(n, 0);
        for (int i = 0; i < n; ++i) {
            int parent = newState->parentIndices[i];
            if (parent >= 0 && parent < n) {
                newState->childrenOf[parent].push_back(i);
                newState->initialPendingCounts[parent]++;
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
