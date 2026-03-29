#pragma once
#include <JuceHeader.h>
#include "AudioClock.h"
#include "TransportState.h"
#include "SafetyProcessors.h"
#include "../Routing/MasterMixer.h"
#include "../Routing/PDCManager.h"
#include "../Routing/RoutingMatrix.h"
#include "../Nodes/TrackProcessor.h"
#include "../Nodes/MetronomeProcessor.h"

// OBSERVACIÓN: El Motor de Audio ha sido completamente purgado del juce::ScopedLock 
// y ya no invoca directamente funciones de PianoRollComponent ni PlaylistComponent.
// Toda la comunicación UI -> Audio se realiza vía Atómicos o la RoutingMatrix lock-free.

class AudioEngine {
public:
    AudioClock clock;
    MetronomeProcessor metronome;
    TransportState transportState;
    RoutingMatrix routingMatrix;

    std::atomic<float> masterVolume { 1.0f };

    void prepareToPlay(int samples, double s) {
        clock.prepare(s, samples);
        metronome.prepare(s);
    }

    void releaseResources() {
    }

    void processBlock(const juce::AudioSourceChannelInfo& bufferToFill) noexcept
    {
        bufferToFill.clearActiveBufferRegion();
        
        // 1. Snapshot Atómico / Wait-Free del Grafo de Audio Actual
        auto* topo = routingMatrix.getAudioThreadState();
        if (!topo) return;

        bool isPlayingNow = transportState.isPlaying.load(std::memory_order_relaxed);
        bool isPreviewing = transportState.isPreviewing.load(std::memory_order_relaxed);
        int currentPreview = transportState.previewPitch.load(std::memory_order_relaxed);

        if (isPlayingNow && !clock.wasPlayingLastBlock) {
            for (auto* track : topo->activeTracks) {
                track->pdcBuffer.clear();
                track->pdcWritePtr = 0;
                for (auto* p : track->plugins) {
                    if (p != nullptr && p->isLoaded()) p->prepareToPlay(clock.sampleRate, bufferToFill.numSamples);
                }
            }
        }

        bool isStoppingNow = transportState.isStoppingNow.load(std::memory_order_relaxed);
        if (isStoppingNow) {
            transportState.isStoppingNow.store(false, std::memory_order_relaxed);
        }
        clock.wasPlayingLastBlock = isPlayingNow;

        juce::MidiBuffer previewMidi;
        
        if (isPreviewing) {
            if (currentPreview != clock.lastPreviewPitch) {
                if (clock.lastPreviewPitch != -1) previewMidi.addEvent(juce::MidiMessage::noteOff(1, clock.lastPreviewPitch), 0);
                previewMidi.addEvent(juce::MidiMessage::noteOn(1, currentPreview, 0.8f), 0);
                clock.lastPreviewPitch = currentPreview;
            }
        } else if (clock.lastPreviewPitch != -1) {
            previewMidi.addEvent(juce::MidiMessage::noteOff(1, clock.lastPreviewPitch), 0);
            clock.lastPreviewPitch = -1;
        }

        int hardwareOutChannels = bufferToFill.buffer->getNumChannels();

        // Limpiar los tracks y alistar vúmetros
        for (auto* track : topo->activeTracks) {
            track->audioBuffer.clear();
            track->currentPeakLevelL = 0.0f; 
            track->currentPeakLevelR = 0.0f;
        }

        clock.update(bufferToFill.numSamples, transportState);
        PDCManager::calculateLatencies(topo, transportState);

        // Procesar en reversa (Carpeta hijas antes que padre)
        for (int i = (int)topo->activeTracks.size() - 1; i >= 0; --i) {
            auto* track = topo->activeTracks[i];
            
            // TrackProcessor purgado de setSize
            TrackProcessor::process(track, clock, bufferToFill.numSamples, isPlayingNow, isStoppingNow, previewMidi);
            
            PDCManager::applyDelay(track, bufferToFill.numSamples);
            MasterMixer::processTrackVolumeAndRouting(track, bufferToFill.numSamples, hardwareOutChannels, topo->parentIndices[i], topo->activeTracks, bufferToFill);
        }

        metronome.process(*bufferToFill.buffer, bufferToFill.numSamples, clock, transportState);

        SafetyProcessors::applyNaNKiller(*bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples);
        float mv = masterVolume.load(std::memory_order_relaxed);
        bufferToFill.buffer->applyGain(bufferToFill.startSample, bufferToFill.numSamples, mv);
        SafetyProcessors::applyHardClipper(*bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples, 1.0f);
    }
};
