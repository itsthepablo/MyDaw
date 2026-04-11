#include "AudioEngine.h"
#include "SafetyProcessors.h"
#include "../Routing/MasterMixer.h"
#include "../Routing/PDCManager.h"
#include "../Nodes/TrackProcessor.h"
#include "../../Mixer/Bridges/MixerParameterBridge.h"
#include "../../Mixer/DSP/MixerDSP.h"

void AudioEngine::prepareToPlay(int samples, double s) {
    currentSampleRate = s; 
    clock.prepare(s, samples);
    metronome.prepare(s);
    threadPool.prepare(); 
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

    bool isPlayingNow = transportState.isPlaying.load(std::memory_order_relaxed);
    bool isPreviewing = transportState.isPreviewing.load(std::memory_order_relaxed);
    int currentPreview = transportState.previewPitch.load(std::memory_order_relaxed);

    PDCManager::dbgTracks.store((int)topo->activeTracks.size(), std::memory_order_relaxed);
    PDCManager::dbgPlaying.store(isPlayingNow ? 1 : 0, std::memory_order_relaxed);

    if (isPlayingNow && !clock.wasPlayingLastBlock) {
        for (auto* track : topo->activeTracks) {
            track->dsp.pdcBuffer.clear();
            track->dsp.pdcWritePtr = 0;
            for (auto* p : track->plugins) {
                if (p != nullptr && p->isLoaded())
                    p->reset();
            }
        }
        if (masterTrack != nullptr) {
            masterTrack->dsp.pdcBuffer.clear();
            masterTrack->dsp.pdcWritePtr = 0;
            for (auto* p : masterTrack->plugins) {
                if (p != nullptr && p->isLoaded())
                    p->reset();
            }
        }
    }

    bool isStoppingNow = transportState.isStoppingNow.load(std::memory_order_relaxed);
    
    if (!isPlayingNow && clock.wasPlayingLastBlock) {
        isStoppingNow = true;
    }

    if (isStoppingNow)
        transportState.isStoppingNow.store(false, std::memory_order_relaxed);

    clock.wasPlayingLastBlock = isPlayingNow;

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

    threadPool.processTracksParallel(
        topo, clock, bufferToFill.numSamples,
        isPlayingNow, isStoppingNow, previewMidi,
        hardwareOutChannels);

    for (int i = 0; i < (int)topo->activeTracks.size(); ++i) {
        int parentIdx = (i < (int)topo->parentIndices.size()) ? topo->parentIndices[i] : -1;
        if (parentIdx == -1) {
            if (masterTrack != nullptr) {
                MasterMixer::routeToTrack(topo->activeTracks[i], masterTrack, bufferToFill.numSamples);
            } else {
                MasterMixer::routeToMaster(topo->activeTracks[i],
                    bufferToFill.numSamples,
                    hardwareOutChannels,
                    bufferToFill);
            }
        }
    }

    if (masterTrack != nullptr) {
        TrackProcessor::process(masterTrack, clock, bufferToFill.numSamples, isPlayingNow, isStoppingNow, previewMidi, topo);
        MixerDSP::applyGainAndPan(masterTrack, bufferToFill.numSamples, hardwareOutChannels);
        
        MasterMixer::routeToMaster(masterTrack, bufferToFill.numSamples, hardwareOutChannels, bufferToFill);

        juce::AudioBuffer<float> analysisProxy(bufferToFill.buffer->getArrayOfWritePointers(), 
                                              bufferToFill.buffer->getNumChannels(), 
                                              bufferToFill.startSample, 
                                              bufferToFill.numSamples);

        for (auto* t : topo->activeTracks) {
            if (t->getType() == TrackType::Loudness) t->dsp.postLoudness.process(analysisProxy);
            else if (t->getType() == TrackType::Balance) t->dsp.postBalance.process(analysisProxy);
            else if (t->getType() == TrackType::MidSide) t->dsp.postMidSide.process(analysisProxy);
        }
    }

    metronome.process(*bufferToFill.buffer, bufferToFill.numSamples, clock, transportState);

    SafetyProcessors::applyNaNKiller(*bufferToFill.buffer,
        bufferToFill.startSample, bufferToFill.numSamples);

    if (clock.justSeeked) {
        bufferToFill.buffer->applyGainRamp(bufferToFill.startSample,
            bufferToFill.numSamples, 0.0f, 1.0f);
        clock.justSeeked = false;
    }

    SafetyProcessors::applyHardClipper(*bufferToFill.buffer,
        bufferToFill.startSample,
        bufferToFill.numSamples, 1.0f);
}
