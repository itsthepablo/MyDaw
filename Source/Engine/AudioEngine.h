#pragma once
#include <JuceHeader.h>
#include "AudioClock.h"
#include "TrackProcessor.h"
#include "MasterMixer.h"
#include "SafetyProcessors.h"

class TrackContainer;
class PianoRollComponent;
class PlaylistComponent;
class MixerComponent;

class AudioEngine {
public:
    // Pool de hilos dedicada al procesamiento de plugins. 
    // Usamos el número de núcleos lógicos del sistema.
    AudioEngine() : threadPool(juce::SystemStats::getNumCpus()) {}

    AudioClock clock;

    void prepareToPlay(int samples, double s, TrackContainer& trackContainer, juce::CriticalSection& audioMutex) {
        clock.prepare(s, samples);
        const juce::ScopedLock sl(audioMutex);
        for (auto* track : trackContainer.getTracks())
            for (auto* p : track->plugins)
                if (p->isLoaded()) p->prepareToPlay(s, samples);
    }

    void releaseResources() {}

    void processBlock(const juce::AudioSourceChannelInfo& bufferToFill,
        TrackContainer& trackContainer,
        PianoRollComponent& pianoRollUI,
        PlaylistComponent& playlistUI,
        MixerComponent& mixerUI,
        juce::CriticalSection& audioMutex)
    {
        // 1. Fase de preparación (Bloqueo mínimo para obtener referencias)
        const juce::ScopedLock sl(audioMutex);

        bufferToFill.clearActiveBufferRegion();
        bool isPlayingNow = pianoRollUI.getIsPlaying();
        auto& tracks = trackContainer.getTracks();
        int hardwareOutChannels = bufferToFill.buffer->getNumChannels();

        // 2. MIDI Panic y Reloj
        if (!isPlayingNow && clock.wasPlayingLastBlock) {
            for (auto* track : tracks) {
                juce::MidiBuffer panic;
                for (int ch = 1; ch <= 16; ++ch) panic.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
                for (auto* p : track->plugins) if (p->isLoaded()) p->processBlock(*bufferToFill.buffer, panic);
            }
        }
        clock.wasPlayingLastBlock = isPlayingNow;

        float finalLoopEnd = juce::jmax(pianoRollUI.getLoopEndPos(), playlistUI.getLoopEndPos());
        clock.update(bufferToFill.numSamples, isPlayingNow, pianoRollUI.getPlayheadPos(), finalLoopEnd, pianoRollUI.getBpm());

        // 3. PARALELISMO: Procesamiento de Plugins
        juce::WaitableEvent processingFinished;
        std::atomic<int> tracksRemaining((int)tracks.size());

        juce::MidiBuffer previewMidi;

        for (auto* track : tracks) {
            // Limpiar buffers de pista antes de procesar
            track->audioBuffer.setSize(juce::jmax(2, hardwareOutChannels), bufferToFill.numSamples);
            track->audioBuffer.clear();

            // CORRECCIÓN E0308: Especificamos explícitamente el tipo std::function para evitar la ambigüedad
            threadPool.addJob(std::function<juce::ThreadPoolJob::JobStatus()>([track, this, isPlayingNow, &previewMidi, &pianoRollUI, finalLoopEnd, &tracksRemaining, &processingFinished]() {
                bool isPianoRollActive = (&(track->notes) == pianoRollUI.getActiveNotesPointer());

                // Procesamiento en hilos paralelos
                TrackProcessor::process(track, clock, clock.blockSize, isPlayingNow, previewMidi, isPianoRollActive, finalLoopEnd);

                if (--tracksRemaining <= 0)
                    processingFinished.signal();

                return juce::ThreadPoolJob::jobHasFinished;
                }));
        }

        // Esperar a que los hilos terminen (Timeout de seguridad para evitar cuelgues)
        processingFinished.wait(20);

        // 4. MEZCLA (Routing): Secuencial para respetar la jerarquía de carpetas
        std::vector<int> parentIndices(tracks.size(), -1);
        std::vector<int> parentStack;
        for (int i = 0; i < (int)tracks.size(); ++i) {
            if (!parentStack.empty()) parentIndices[i] = parentStack.back();
            if (tracks[i]->folderDepthChange == 1) parentStack.push_back(i);
            else if (tracks[i]->folderDepthChange < 0) {
                int drops = -tracks[i]->folderDepthChange;
                for (int d = 0; d < drops; ++d) if (!parentStack.empty()) parentStack.pop_back();
            }
        }

        for (int i = (int)tracks.size() - 1; i >= 0; --i) {
            MasterMixer::processTrackVolumeAndRouting(tracks[i], bufferToFill.numSamples, hardwareOutChannels, parentIndices[i], tracks, bufferToFill);
        }

        // 5. Finalización y Seguridad
        if (isPlayingNow) {
            float nPh = clock.nextPh;
            juce::MessageManager::callAsync([&pianoRollUI, &playlistUI, nPh]() {
                pianoRollUI.setPlayheadPos(nPh);
                playlistUI.setPlayheadPos(nPh);
                });
        }

        SafetyProcessors::applyNaNKiller(*bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples);
        bufferToFill.buffer->applyGain(bufferToFill.startSample, bufferToFill.numSamples, mixerUI.getMasterVolume());
        SafetyProcessors::applyHardClipper(*bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples, 1.0f);
    }

private:
    juce::ThreadPool threadPool;
};