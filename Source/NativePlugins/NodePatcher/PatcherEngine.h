#pragma once
#include <JuceHeader.h>

class NodePatcher;

// ==============================================================================
// MOTOR DSP DEL NODE PATCHER
// Maneja el ordenamiento topológico y el ruteo de buffers entre plugins paralelos.
// ==============================================================================
class PatcherEngine {
public:
    PatcherEngine(NodePatcher* ownerPatcher);
    
    void prepareToPlay(double sampleRate, int blockSize);
    void processBlock(juce::AudioBuffer<float>& mainBuffer, juce::MidiBuffer& midiMessages);
    
    // Reconstruye el grafo de ruteo DSP basado en los nodos y conexiones actuales.
    // Llama a esto cada vez que se añade/borra un nodo o cable.
    void compileGraph(); 

private:
    NodePatcher* owner;
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    struct DSPNode {
        juce::Uuid id;
        juce::AudioBuffer<float> buffer;
        std::vector<juce::Uuid> incomingConnections; // IDs de las conexiones que entran aquí
        bool isInput = false;
        bool isOutput = false;
    };

    std::vector<juce::Uuid> processingOrder;
    std::map<juce::Uuid, std::unique_ptr<DSPNode>> dspNodes;
    
    juce::CriticalSection lock;
};
