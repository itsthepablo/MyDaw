#pragma once
#include <JuceHeader.h>
#include <atomic>
#include "../../Tracks/Track.h"

class RoutingMatrix {
public:
    struct TopoState {
        juce::Array<Track*> activeTracks;
        std::vector<int> parentIndices;
    };

    // Puntero atómico hacia la topología activa real
    std::atomic<TopoState*> currentState { nullptr };

    // Función llamada desde el Hilo de la UI (Message Thread) cuando se altera la lista
    void commitNewTopology(const juce::OwnedArray<Track>& tracks) {
        auto* newState = new TopoState();
        newState->activeTracks.addArray(tracks);
        
        newState->parentIndices.resize(tracks.size(), -1);
        std::vector<int> parentStack;
        for (int i = 0; i < tracks.size(); ++i) {
            if (!parentStack.empty()) newState->parentIndices[i] = parentStack.back();
            if (tracks[i]->folderDepthChange == 1) parentStack.push_back(i);
            else if (tracks[i]->folderDepthChange < 0) {
                int drops = -tracks[i]->folderDepthChange;
                for(int d=0; d<drops; ++d) { if(!parentStack.empty()) parentStack.pop_back(); }
            }
        }
        
        auto* oldState = currentState.exchange(newState);
        if (oldState) {
            juce::MessageManager::callAsync([oldState]() { delete oldState; }); // Lock-free cleanup
        }
    }
    
    // Función llamada desde el Hilo de Audio. Estrictamente Wait-Free.
    TopoState* getAudioThreadState() const noexcept {
        return currentState.load(std::memory_order_acquire);
    }
    
    ~RoutingMatrix() {
        auto* old = currentState.exchange(nullptr);
        if (old) delete old;
    }
};
