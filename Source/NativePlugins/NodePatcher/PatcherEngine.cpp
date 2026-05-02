#include "PatcherEngine.h"
#include "NodePatcher.h"

PatcherEngine::PatcherEngine(NodePatcher* ownerPatcher) : owner(ownerPatcher) {}

void PatcherEngine::prepareToPlay(double sampleRate, int blockSize)
{
    juce::ScopedLock sl(lock);
    currentSampleRate = sampleRate;
    currentBlockSize = blockSize > 0 ? blockSize : 512;
    compileGraph();
}

void PatcherEngine::compileGraph()
{
    juce::ScopedLock sl(lock);
    
    dspNodes.clear();
    processingOrder.clear();

    std::map<juce::Uuid, int> inDegree;
    std::map<juce::Uuid, std::vector<juce::Uuid>> adjList;

    // 1. Inicializar nodos de DSP locales
    for (auto& n : owner->nodes) {
        inDegree[n.id] = 0;
        auto dspNode = std::make_unique<DSPNode>();
        dspNode->id = n.id;
        dspNode->isInput = n.isInput;
        dspNode->isOutput = n.isOutput;
        dspNode->buffer.setSize(2, currentBlockSize, false, true, true);
        dspNodes[n.id] = std::move(dspNode);
        
        // Propagar prepareToPlay al efecto interno si existe
        if (n.effect) {
            n.effect->prepareToPlay(currentSampleRate, currentBlockSize);
        }
    }

    // 2. Construir lista de adyacencia y grados de entrada (Cables)
    for (auto& c : owner->connections) {
        adjList[c.sourceId].push_back(c.destId);
        inDegree[c.destId]++;
        
        if (dspNodes.count(c.destId)) {
            dspNodes[c.destId]->incomingConnections.push_back(c.id);
        }
    }

    // 3. Ordenamiento Topológico (Kahn's Algorithm)
    std::vector<juce::Uuid> queue;
    for (auto& pair : inDegree) {
        if (pair.second == 0) queue.push_back(pair.first);
    }

    while (!queue.empty()) {
        auto curr = queue.front();
        queue.erase(queue.begin());
        processingOrder.push_back(curr);

        for (auto& neighbor : adjList[curr]) {
            inDegree[neighbor]--;
            if (inDegree[neighbor] == 0) {
                queue.push_back(neighbor);
            }
        }
    }
}

void PatcherEngine::processBlock(juce::AudioBuffer<float>& mainBuffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedLock sl(lock);
    if (processingOrder.empty()) return;

    int numChans = mainBuffer.getNumChannels();
    int numSamples = mainBuffer.getNumSamples();

    for (auto& id : processingOrder) {
        auto* dspNode = dspNodes[id].get();
        auto* patcherNode = owner->getNode(id);
        if (!patcherNode || !dspNode) continue;

        dspNode->buffer.setSize(numChans, numSamples, false, false, true);
        dspNode->buffer.clear();

        if (dspNode->isInput) {
            // Audio In: Coger señal principal
            for (int ch = 0; ch < numChans; ++ch)
                dspNode->buffer.copyFrom(ch, 0, mainBuffer, ch, 0, numSamples);
        } 
        else {
            // Nodos intermedios: Mezclar todas las entradas (Cables entrantes)
            for (auto& connId : dspNode->incomingConnections) {
                if (auto* conn = owner->getConnection(connId)) {
                    if (dspNodes.count(conn->sourceId)) {
                        auto& srcBuffer = dspNodes[conn->sourceId]->buffer;
                        float vol = conn->volume;
                        if (srcBuffer.getNumSamples() == numSamples) {
                            for (int ch = 0; ch < std::min(numChans, srcBuffer.getNumChannels()); ++ch) {
                                dspNode->buffer.addFrom(ch, 0, srcBuffer, ch, 0, numSamples, vol);
                            }
                        }
                    }
                }
            }
            
            // Procesar el efecto
            if (patcherNode->effect) {
                patcherNode->effect->processBlock(dspNode->buffer, midiMessages);
            }
        }

        if (dspNode->isOutput) {
            // Audio Out: Escribir al buffer de la pista
            // Aquí podríamos mezclar en lugar de sobrescribir si quisieramos dry/wet global
            for (int ch = 0; ch < numChans; ++ch)
                mainBuffer.copyFrom(ch, 0, dspNode->buffer, ch, 0, numSamples);
        }
    }
}
