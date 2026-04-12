#include "AudioEngine.h"
#include "SafetyProcessors.h"
#include "../Routing/MasterMixer.h"
#include "../Routing/PDCManager.h"
#include "../Nodes/TrackProcessor.h"
#include "../../Mixer/Bridges/MixerParameterBridge.h"
#include "../../Mixer/DSP/MixerDSP.h"
#include "../../NativePlugins/FFTAnalyzerPlugin/SimpleAnalyzer.h"
#include "../../NativePlugins/VUMeter/VUBallistics.h"
#include "../../Data/Track.h"

void AudioEngine::prepareToPlay(int samples, double s) {
    currentSampleRate = s; 
    clock.prepare(s, samples);
    metronome.prepare(s);
    threadPool.prepare(); 
    
    transportGain.reset(s, 0.015); // 15ms ramp (transiciones ultra-suaves)
    transportGain.setCurrentAndTargetValue(0.0f);
}

void AudioEngine::releaseResources() {
    threadPool.shutdown();
}

void AudioEngine::resetForRender() {
    clock.currentSamplePos = 0; 
    clock.currentPh = 0.0f;     
    clock.nextPh = 0.0f;        
    clock.justSeeked = false; 
    clock.wasPlayingLastBlock = false; 
    transportState.isStoppingNow.store(false, std::memory_order_relaxed); 
    auto* topo = routingMatrix.getAudioThreadState();
    if (topo) {
        for (auto* track : topo->activeTracks) {
            track->audioBuffer.clear();
            track->dsp.pdcBuffer.clear();
            track->dsp.pdcWritePtr = 0;
            for (auto* p : track->plugins) {
                if (p != nullptr && p->isLoaded())
                    p->reset();
            }
        }
    }

    if (masterTrack) {
        masterTrack->audioBuffer.clear();
        masterTrack->dsp.pdcBuffer.clear();
        masterTrack->dsp.pdcWritePtr = 0;
        for (auto* p : masterTrack->plugins) {
            if (p != nullptr && p->isLoaded())
                p->reset();
        }
    }
}

void AudioEngine::setNonRealtime(bool isNonRealtime) {
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

void AudioEngine::processBlock(const juce::AudioSourceChannelInfo& bufferToFill) noexcept
{
    juce::ScopedNoDenormals noDenormals;
    bufferToFill.clearActiveBufferRegion();

    auto* topo = routingMatrix.getAudioThreadState();
    if (!topo) return;

    // --- ESTADO DE TRANSPORTE ---
    const bool isPlayingNow = transportState.isPlaying.load(std::memory_order_relaxed);
    
    // Gestión de ganancia suavizada (Anti-Clics)
    transportGain.setTargetValue(isPlayingNow ? 1.0f : 0.0f);
    const bool isFadingOut = !isPlayingNow && transportGain.isSmoothing();
    const bool shouldProcess = isPlayingNow || isFadingOut;

    // --- RESET DE VSTs AL ARRANCAR ---
    if (isPlayingNow && !clock.wasPlayingLastBlock) {
        for (auto* track : topo->activeTracks) {
            track->dsp.pdcBuffer.clear();
            track->dsp.pdcWritePtr = 0;
            for (auto* p : track->plugins) {
                if (p != nullptr && p->isLoaded()) p->reset();
            }
        }
        if (masterTrack) {
            masterTrack->dsp.pdcBuffer.clear();
            masterTrack->dsp.pdcWritePtr = 0;
            for (auto* p : masterTrack->plugins) {
                if (p != nullptr && p->isLoaded()) p->reset();
            }
        }
    }

    // Actualizamos estado para el siguiente bloque
    clock.wasPlayingLastBlock = isPlayingNow;

    if (!shouldProcess) {
        // Motor detenido totalmente y ganancia en cero.
        transportGain.setCurrentAndTargetValue(0.0f);
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    // --- PROCESAMIENTO ACTIVO ---
    bool isPreviewing = transportState.isPreviewing.load(std::memory_order_relaxed);
    int currentPreview = transportState.previewPitch.load(std::memory_order_relaxed);

    // Reloj (Avanza si está tocando o en fade-out)
    clock.update(bufferToFill.numSamples, transportState, isFadingOut);

    juce::MidiBuffer previewMidi;
    if (isPreviewing) {
        if (currentPreview != clock.lastPreviewPitch) {
            if (clock.lastPreviewPitch != -1)
                previewMidi.addEvent(juce::MidiMessage::noteOff(1, clock.lastPreviewPitch), 0);
            previewMidi.addEvent(juce::MidiMessage::noteOn(1, currentPreview, 0.8f), 0);
            clock.lastPreviewPitch = currentPreview;
        }
    } else if (clock.lastPreviewPitch != -1) {
        previewMidi.addEvent(juce::MidiMessage::noteOff(1, clock.lastPreviewPitch), 0);
        clock.lastPreviewPitch = -1;
    }

    int hardwareOutChannels = bufferToFill.buffer->getNumChannels();

    for (auto* track : topo->activeTracks) {
        track->audioBuffer.clear();
        MixerParameterBridge::resetPeakLevels(track);
    }
    if (masterTrack != nullptr) {
        masterTrack->audioBuffer.clear();
        MixerParameterBridge::resetPeakLevels(masterTrack);
    }

    PDCManager::calculateLatencies(topo, transportState);

    // Aquí usamos isFadingOut como el flag isStoppingNow para que los tracks sepan que deben terminar suavemente
    threadPool.processTracksParallel(
        topo, clock, bufferToFill.numSamples,
        isPlayingNow, isFadingOut, previewMidi,
        hardwareOutChannels);

    for (int i = 0; i < (int)topo->activeTracks.size(); ++i) {
        int parentIdx = (i < (int)topo->parentIndices.size()) ? topo->parentIndices[i] : -1;
        if (parentIdx == -1) {
            if (masterTrack != nullptr) MasterMixer::routeToTrack(topo->activeTracks[i], masterTrack, bufferToFill.numSamples);
            else MasterMixer::routeToMaster(topo->activeTracks[i], bufferToFill.numSamples, hardwareOutChannels, bufferToFill);
        }
    }

    if (masterTrack != nullptr) {
        TrackProcessor::process(masterTrack, clock, bufferToFill.numSamples, isPlayingNow, isFadingOut, previewMidi, topo);
        MixerDSP::applyGainAndPan(masterTrack, bufferToFill.numSamples, hardwareOutChannels);
        MasterMixer::routeToMaster(masterTrack, bufferToFill.numSamples, hardwareOutChannels, bufferToFill);
    }

    metronome.process(*bufferToFill.buffer, bufferToFill.numSamples, clock, transportState);

    SafetyProcessors::applyNaNKiller(*bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples);

    if (clock.justSeeked) {
        bufferToFill.buffer->applyGainRamp(bufferToFill.startSample, bufferToFill.numSamples, 0.0f, 1.0f);
        clock.justSeeked = false;
    }

    // --- APLICACIÓN DE GANANCIA SUAVIZADA (Anti-Clic Final) ---
    transportGain.applyGain(*bufferToFill.buffer, bufferToFill.numSamples);

    SafetyProcessors::applyHardClipper(*bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples, 1.0f);

    // --- MONITOR TAP ---
    auto* analyzerPtr = static_cast<SimpleAnalyzer*>(analyzerToFeed.load(std::memory_order_relaxed));
    auto* vuPtr = static_cast<VUBallistics*>(vuToFeed.load(std::memory_order_relaxed));
    if (analyzerPtr || vuPtr) {
        const float* audioL = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);
        const float* audioR = (bufferToFill.buffer->getNumChannels() > 1) ? bufferToFill.buffer->getReadPointer(1, bufferToFill.startSample) : audioL;
        if (analyzerPtr) analyzerPtr->pushBuffer(audioL, bufferToFill.numSamples);
        if (vuPtr) vuPtr->processStereo(audioL, audioR, bufferToFill.numSamples);
    }
}
