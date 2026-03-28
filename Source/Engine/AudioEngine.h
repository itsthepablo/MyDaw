#pragma once
#include <JuceHeader.h>
#include "AudioClock.h"
#include "TrackProcessor.h"
#include "MasterMixer.h"
#include "SafetyProcessors.h"
#include "Metronome.h" 
#include "PDCManager.h" // <-- CONECTAMOS EL NUEVO MÓDULO

class TrackContainer;
class PianoRollComponent;
class PlaylistComponent;
class MixerComponent;

class AudioEngine {
public:
    AudioClock clock;
    Metronome metronome;

    void prepareToPlay(int samples, double s, TrackContainer& trackContainer, juce::CriticalSection& audioMutex) {
        clock.prepare(s, samples);
        metronome.prepare(s);

        const juce::ScopedLock sl(audioMutex);
        for (auto* track : trackContainer.getTracks()) {
            track->audioBuffer.setSize(16, samples, false, true, true);
            track->instrumentMixBuffer.setSize(2, samples, false, true, true);
            track->tempSynthBuffer.setSize(2, samples, false, true, true);
            track->midSideBuffer.setSize(1, samples, false, true, true);
            
            // Reservamos memoria para soportar hasta ~3 segundos de retraso por VSTs pesados
            track->pdcBuffer.setSize(2, 131072, false, true, true);
            track->pdcBuffer.clear();
            track->pdcWritePtr = 0;

            for (auto* p : track->plugins)
                if (p->isLoaded()) p->prepareToPlay(s, samples);
        }
    }

    void releaseResources() {
    }

    void processBlock(const juce::AudioSourceChannelInfo& bufferToFill,
        TrackContainer& trackContainer,
        PianoRollComponent& pianoRollUI,
        PlaylistComponent& playlistUI,
        MixerComponent& mixerUI,
        juce::CriticalSection& audioMutex)
    {
        const juce::ScopedLock sl(audioMutex);
        bufferToFill.clearActiveBufferRegion();
        bool isPlayingNow = pianoRollUI.getIsPlaying();

        if (!isPlayingNow && clock.wasPlayingLastBlock) {
            juce::MidiBuffer masterPanic;
            masterPanic.ensureSize(8192); 
            for (int ch = 1; ch <= 16; ++ch) {
                masterPanic.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
                masterPanic.addEvent(juce::MidiMessage::controllerEvent(ch, 64, 0), 0); 
                for (int note = 0; note < 128; ++note) {
                    masterPanic.addEvent(juce::MidiMessage::noteOff(ch, note), 0);
                }
            }

            for (auto* track : trackContainer.getTracks()) {
                for (auto* p : track->plugins) {
                    if (p->isLoaded()) {
                        juce::MidiBuffer clonedPanic;
                        clonedPanic.ensureSize(8192);
                        clonedPanic.addEvents(masterPanic, 0, -1, 0);
                        p->processBlock(*bufferToFill.buffer, clonedPanic);
                    }
                }
            }
        }
        clock.wasPlayingLastBlock = isPlayingNow;

        juce::MidiBuffer previewMidi;
        previewMidi.ensureSize(1024);
        int currentPreview = pianoRollUI.getPreviewPitch();
        if (pianoRollUI.getIsPreviewing()) {
            if (currentPreview != clock.lastPreviewPitch) {
                if (clock.lastPreviewPitch != -1) previewMidi.addEvent(juce::MidiMessage::noteOff(1, clock.lastPreviewPitch), 0);
                previewMidi.addEvent(juce::MidiMessage::noteOn(1, currentPreview, 0.8f), 0);
                clock.lastPreviewPitch = currentPreview;
            }
        }
        else if (clock.lastPreviewPitch != -1) {
            previewMidi.addEvent(juce::MidiMessage::noteOff(1, clock.lastPreviewPitch), 0);
            clock.lastPreviewPitch = -1;
        }

        int hardwareOutChannels = bufferToFill.buffer->getNumChannels();
        auto& tracks = trackContainer.getTracks();

        for (auto* track : tracks) {
            int requiredChannels = juce::jmax(16, hardwareOutChannels);
            track->audioBuffer.setSize(requiredChannels, bufferToFill.numSamples, false, false, true);
            track->audioBuffer.clear();
            track->currentPeakLevelL = 0.0f; track->currentPeakLevelR = 0.0f;
        }

        float finalLoopEnd = juce::jmax(pianoRollUI.getLoopEndPos(), playlistUI.getLoopEndPos());
        clock.update(bufferToFill.numSamples, isPlayingNow, pianoRollUI.getPlayheadPos(), finalLoopEnd, pianoRollUI.getBpm());

        // ---> ESCANEO DE LATENCIA GLOBAL <---
        PDCManager::calculateLatencies(trackContainer);

        std::vector<int> parentIndices(tracks.size(), -1);
        std::vector<int> parentStack;
        for (int i = 0; i < (int)tracks.size(); ++i) {
            if (!parentStack.empty()) parentIndices[i] = parentStack.back();
            if (tracks[i]->folderDepthChange == 1) parentStack.push_back(i);
            else if (tracks[i]->folderDepthChange < 0) {
                int drops = -tracks[i]->folderDepthChange;
                for (int d = 0; d < drops; ++d) { if (!parentStack.empty()) parentStack.pop_back(); }
            }
        }

        MidiClipData* activeClip = pianoRollUI.getActiveClip();

        for (int i = (int)tracks.size() - 1; i >= 0; --i) {
            auto* track = tracks[i];
            bool isPianoRollActive = false;

            if (track->isSelected) {
                isPianoRollActive = true;
            }
            else if (activeClip != nullptr) {
                for (auto* clip : track->midiClips) {
                    if (clip == activeClip) {
                        isPianoRollActive = true;
                        break;
                    }
                }
            }

            TrackProcessor::process(track, clock, bufferToFill.numSamples, isPlayingNow, previewMidi, isPianoRollActive, finalLoopEnd);
            MasterMixer::processTrackVolumeAndRouting(track, bufferToFill.numSamples, hardwareOutChannels, parentIndices[i], tracks, bufferToFill);
        }

        metronome.process(*bufferToFill.buffer, bufferToFill.numSamples, clock, pianoRollUI.getBpm());

        SafetyProcessors::applyNaNKiller(*bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples);
        bufferToFill.buffer->applyGain(bufferToFill.startSample, bufferToFill.numSamples, mixerUI.getMasterVolume());
        SafetyProcessors::applyHardClipper(*bufferToFill.buffer, bufferToFill.startSample, bufferToFill.numSamples, 1.0f);
    }
};