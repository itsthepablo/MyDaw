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
#include "../Threading/AudioThreadPool.h"
#include "../../Mixer/Bridges/MixerParameterBridge.h"
#include "../../Mixer/DSP/MixerDSP.h"

// ==============================================================================
// AudioEngine — Motor central de procesamiento de audio multihilo.
//
// ARQUITECTURA DE DOS FASES POR BLOQUE:
//
//  FASE 1 — PARALELA (AudioThreadPool, todos los nucleos):
//    Para cada track (en orden de dependencias, hijos antes que padres):
//      - Acumular hijos completados en buffer del padre
//      - TrackProcessor::process  (DSP, clips, plugins, sintetizadores)
//      - PDCManager::applyDelay   (compensacion de latencia)
//      - MasterMixer::applyGainAndPan  (volumen + balance)
//    El resultado de cada track queda en track->audioBuffer.
//
//  FASE 2 — SECUENCIAL (audio thread principal, microsegundos):
//    Para cada track raiz (sin carpeta padre):
//      - MasterMixer::routeToMaster  (suma al buffer de hardware)
//
//  FASE 3 — POST-PROCESO (audio thread principal):
//    - Metronomo, NaN killer, ganancia maestra, hard clipper
// ==============================================================================

class AudioEngine {
public:
    AudioClock clock;
    MetronomeProcessor metronome;
    TransportState transportState;
    RoutingMatrix routingMatrix;
    AudioThreadPool threadPool;


    // --- INYECCIÓN 1: Captura de Sample Rate para exportación ---
    double currentSampleRate = 44100.0;

    void prepareToPlay(int samples, double s) {
        currentSampleRate = s; // --- INYECCIÓN 2 ---
        clock.prepare(s, samples);
        metronome.prepare(s);
        threadPool.prepare(); // Auto-detect hardware_concurrency() - 1 workers
    }

    void releaseResources() {
        threadPool.shutdown();
    }

    // --- INYECCIÓN 3: Método puro para limpiar el motor antes de exportar ---
    void resetForRender() {
        clock.currentSamplePos = 0; // CORREGIDO
        clock.justSeeked = true;

        auto* topo = routingMatrix.getAudioThreadState();
        if (topo) {
            for (auto* track : topo->activeTracks) {
                track->audioBuffer.clear();
                track->pdcBuffer.clear();
                track->pdcWritePtr = 0;
                for (auto* p : track->plugins) {
                    if (p != nullptr && p->isLoaded())
                        p->reset();
                }
            }
        }

        if (masterTrack) {
            masterTrack->audioBuffer.clear();
            masterTrack->pdcBuffer.clear();
            masterTrack->pdcWritePtr = 0;
            for (auto* p : masterTrack->plugins) {
                if (p != nullptr && p->isLoaded())
                    p->reset();
            }
        }
    }

    void setNonRealtime(bool isNonRealtime) {
        auto* topo = routingMatrix.getAudioThreadState();
        if (topo) {
            for (auto* track : topo->activeTracks) {
                for (auto* p : track->plugins) {
                    if (p != nullptr && p->isLoaded())
                        p->setNonRealtime(isNonRealtime);
                }
            }
        }
    }
    // -------------------------------------------------------------------------

    Track* masterTrack = nullptr;

    void setMasterTrack(Track* t) { masterTrack = t; }

    void processBlock(const juce::AudioSourceChannelInfo& bufferToFill) noexcept
    {
        // =========================================================================
        // PROTECCION DSP CRITICA: Evita que plugins pesados colapsen la CPU y el 
        // audio generen subnormales/denormals. Actua sobre el thread principal.
        // =========================================================================
        juce::ScopedNoDenormals noDenormals;

        bufferToFill.clearActiveBufferRegion();

        // 1. Snapshot atomico / Wait-Free del grafo de audio actual
        auto* topo = routingMatrix.getAudioThreadState();
        if (!topo) return;

        bool isPlayingNow = transportState.isPlaying.load(std::memory_order_relaxed);
        bool isPreviewing = transportState.isPreviewing.load(std::memory_order_relaxed);
        int currentPreview = transportState.previewPitch.load(std::memory_order_relaxed);

        PDCManager::dbgTracks.store((int)topo->activeTracks.size(), std::memory_order_relaxed);
        PDCManager::dbgPlaying.store(isPlayingNow ? 1 : 0, std::memory_order_relaxed);

        // Resetear colas de plugins (Reverbs, Delays) al iniciar la reproduccion
        // NUNCA llamar a prepareToPlay aquí porque destruye el loop real-time y cuelga VSTs pesados.
        if (isPlayingNow && !clock.wasPlayingLastBlock) {
            for (auto* track : topo->activeTracks) {
                track->pdcBuffer.clear();
                track->pdcWritePtr = 0;
                for (auto* p : track->plugins) {
                    if (p != nullptr && p->isLoaded())
                        p->reset();
                }
            }
            if (masterTrack != nullptr) {
                masterTrack->pdcBuffer.clear();
                masterTrack->pdcWritePtr = 0;
                for (auto* p : masterTrack->plugins) {
                    if (p != nullptr && p->isLoaded())
                        p->reset();
                }
            }
        }

        bool isStoppingNow = transportState.isStoppingNow.load(std::memory_order_relaxed);
        
        // --- LIMPIEZA MIDI EN PAUSA/STOP --- 
        // Si acabamos de dejar de tocar (Pausa o Stop manual), forzamos isStoppingNow por un bloque
        if (!isPlayingNow && clock.wasPlayingLastBlock) {
            isStoppingNow = true;
        }

        if (isStoppingNow)
            transportState.isStoppingNow.store(false, std::memory_order_relaxed);

        clock.wasPlayingLastBlock = isPlayingNow;

        // Construir MIDI de preview (teclado virtual on-screen)
        juce::MidiBuffer previewMidi;
        if (isPreviewing) {
            if (currentPreview != clock.lastPreviewPitch) {
                if (clock.lastPreviewPitch != -1)
                    previewMidi.addEvent(juce::MidiMessage::noteOff(1, clock.lastPreviewPitch), 0);
                previewMidi.addEvent(juce::MidiMessage::noteOn(1, currentPreview, 0.8f), 0);
                clock.lastPreviewPitch = currentPreview;
            }
        }
        else if (clock.lastPreviewPitch != -1) {
            previewMidi.addEvent(juce::MidiMessage::noteOff(1, clock.lastPreviewPitch), 0);
            clock.lastPreviewPitch = -1;
        }

        int hardwareOutChannels = bufferToFill.buffer->getNumChannels();

        // Limpiar buffers y vumétros de todos los tracks (sequentially — rapido)
        for (auto* track : topo->activeTracks) {
            track->audioBuffer.clear();
            MixerParameterBridge::resetPeakLevels(track);
        }

        if (masterTrack != nullptr) {
            masterTrack->audioBuffer.clear();
            MixerParameterBridge::resetPeakLevels(masterTrack);
        }

        clock.update(bufferToFill.numSamples, transportState);
        PDCManager::calculateLatencies(topo, transportState);

        // =======================================================
        // FASE 1: PROCESAMIENTO PARALELO (todos los nucleos)
        // El AudioThreadPool resuelve el grafo de dependencias
        // y procesa tracks en paralelo respetando padre-hijo.
        // =======================================================
        threadPool.processTracksParallel(
            topo, clock, bufferToFill.numSamples,
            isPlayingNow, isStoppingNow, previewMidi,
            hardwareOutChannels);

        // =======================================================
        // FASE 2: ROUTING AL MASTER (secuencial, instantaneo)
        // Solo los tracks raiz (sin carpeta padre) van al master.
        // Los tracks hija ya acumularon su audio en el padre
        // durante la Fase 1 (ver AudioThreadPool::processOneTask).
        // =======================================================
        for (int i = 0; i < (int)topo->activeTracks.size(); ++i) {
            int parentIdx = (i < (int)topo->parentIndices.size()) ? topo->parentIndices[i] : -1;
            if (parentIdx == -1) {
                if (masterTrack != nullptr) {
                    // Sumar al Master Track
                    MasterMixer::routeToTrack(topo->activeTracks[i], masterTrack, bufferToFill.numSamples);
                } else {
                    // Fallback: Ir directamente al hardware
                    MasterMixer::routeToMaster(topo->activeTracks[i],
                        bufferToFill.numSamples,
                        hardwareOutChannels,
                        bufferToFill);
                }
            }
        }

        // =======================================================
        // FASE 2.5: PROCESAR EL MASTER TRACK (Global FX & Gain)
        // =======================================================
        if (masterTrack != nullptr) {
            TrackProcessor::process(masterTrack, clock, bufferToFill.numSamples, isPlayingNow, isStoppingNow, previewMidi, topo);
            MixerDSP::applyGainAndPan(masterTrack, bufferToFill.numSamples, hardwareOutChannels);
            
            // Volcar Master Track al buffer de hardware final
            MasterMixer::routeToMaster(masterTrack, bufferToFill.numSamples, hardwareOutChannels, bufferToFill);

            // =======================================================
            // FASE 2.6: ALIMENTAR TRACKS DE ANÁLISIS (SISTEMA)
            // =======================================================
            // Extraer la región del buffer a analizar (teniendo en cuenta startSample)
            juce::AudioBuffer<float> analysisProxy(bufferToFill.buffer->getArrayOfWritePointers(), 
                                                  bufferToFill.buffer->getNumChannels(), 
                                                  bufferToFill.startSample, 
                                                  bufferToFill.numSamples);

            for (auto* t : topo->activeTracks) {
                if (t->getType() == TrackType::Loudness) t->postLoudness.process(analysisProxy);
                else if (t->getType() == TrackType::Balance) t->postBalance.process(analysisProxy);
                else if (t->getType() == TrackType::MidSide) t->postMidSide.process(analysisProxy);
            }
        }

        // =======================================================
        // FASE 3: POST-PROCESO MAESTRO
        // =======================================================
        metronome.process(*bufferToFill.buffer, bufferToFill.numSamples, clock, transportState);

        SafetyProcessors::applyNaNKiller(*bufferToFill.buffer,
            bufferToFill.startSample, bufferToFill.numSamples);


        // Anti-click (De-zipper) en seek
        if (clock.justSeeked) {
            bufferToFill.buffer->applyGainRamp(bufferToFill.startSample,
                bufferToFill.numSamples, 0.0f, 1.0f);
            clock.justSeeked = false;
        }

        SafetyProcessors::applyHardClipper(*bufferToFill.buffer,
            bufferToFill.startSample,
            bufferToFill.numSamples, 1.0f);
    }
};