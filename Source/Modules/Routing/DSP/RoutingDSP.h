#pragma once
#include <JuceHeader.h>
#include "../Data/RoutingData.h"
#include "../../../Tracks/Track.h"

/**
 * Motor de procesamiento para ruteo de audio (Sends y Sidechain).
 */
class RoutingDSP {
public:
    /**
     * Recolecta todos los IDs de pista de los cuales dependen los plugins de este track (Sidechain).
     */
    static std::vector<int> getSidechainDependencies(const juce::OwnedArray<BaseEffect>& plugins) {
        std::vector<int> deps;
        for (auto* p : plugins) {
            if (p != nullptr) {
                int sourceId = p->sidechain.sourceTrackId.load(std::memory_order_relaxed);
                if (sourceId != -1) deps.push_back(sourceId);
            }
        }
        return deps;
    }

    /**
     * Procesa una lista de envíos para un track determinado.
     */
    static void processSends(const juce::AudioBuffer<float>& sourceBuffer,
                             const std::vector<SendEntry>& sends,
                             const std::map<int, int>& idToIndex,
                             const juce::Array<Track*>& activeTracks,
                             bool preFader,
                             juce::AudioBuffer<float>& msWorkBuffer, 
                             int numSamples)
    {
        bool midCalculated = false;
        bool sideCalculated = false;

        for (const auto& send : sends) {
            if (send.isMuted || send.targetTrackId == -1 || send.isPreFader != preFader) continue;
            if (!idToIndex.count(send.targetTrackId)) continue;

            int targetIdx = idToIndex.at(send.targetTrackId);
            auto* target = activeTracks[targetIdx];
            const juce::SpinLock::ScopedLockType sl(target->bufferLock);

            if (send.mode == RoutingMode::Stereo) {
                int chans = juce::jmin(target->audioBuffer.getNumChannels(), sourceBuffer.getNumChannels());
                for (int c = 0; c < chans; ++c)
                    target->audioBuffer.addFrom(c, 0, sourceBuffer, c, 0, numSamples, send.gain);
            }
            else {
                // Cálculo bajo demanda fuera del bucle de samples
                if (send.mode == RoutingMode::Mid && !midCalculated) {
                    auto* l = sourceBuffer.getReadPointer(0);
                    auto* r = sourceBuffer.getNumChannels() > 1 ? sourceBuffer.getReadPointer(1) : l;
                    auto* w = msWorkBuffer.getWritePointer(0);
                    for (int s = 0; s < numSamples; ++s) w[s] = (l[s] + r[s]) * 0.5f;
                    midCalculated = true;
                    sideCalculated = false;
                }
                else if (send.mode == RoutingMode::Side && !sideCalculated) {
                    auto* l = sourceBuffer.getReadPointer(0);
                    auto* r = sourceBuffer.getNumChannels() > 1 ? sourceBuffer.getReadPointer(1) : l;
                    auto* w = msWorkBuffer.getWritePointer(0);
                    for (int s = 0; s < numSamples; ++s) w[s] = (l[s] - r[s]) * 0.5f;
                    sideCalculated = true;
                    midCalculated = false;
                }

                if (send.mode == RoutingMode::Mid) {
                    for (int c = 0; c < target->audioBuffer.getNumChannels(); ++c)
                        target->audioBuffer.addFrom(c, 0, msWorkBuffer, 0, 0, numSamples, send.gain);
                }
                else if (send.mode == RoutingMode::Side) {
                    if (target->audioBuffer.getNumChannels() > 0)
                        target->audioBuffer.addFrom(0, 0, msWorkBuffer, 0, 0, numSamples, send.gain);
                    if (target->audioBuffer.getNumChannels() > 1)
                        target->audioBuffer.addFrom(1, 0, msWorkBuffer, 0, 0, numSamples, -send.gain);
                }
            }
        }
    }
};
