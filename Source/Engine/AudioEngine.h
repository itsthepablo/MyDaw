#pragma once
#include <JuceHeader.h>
#include "AudioClock.h"
#include "TrackProcessor.h"
#include "MasterMixer.h"

// Forward declarations para evitar errores de compilación circular
class TrackContainer;
class PianoRollComponent;
class PlaylistComponent;
class MixerComponent;

class AudioEngine {
public:
    AudioClock clock;

    void prepareToPlay(int samples, double s, TrackContainer& trackContainer, juce::CriticalSection& audioMutex) {
        clock.prepare(s, samples);
        const juce::ScopedLock sl(audioMutex);
        for (auto* track : trackContainer.getTracks())
            for (auto* p : track->plugins)
                if (p->isLoaded()) p->prepareToPlay(s, samples);
    }

    void releaseResources() {} // Función vacía para evitar errores de llamada en MainComponent

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

        // 1. Pánico MIDI (All Notes Off al detener)
        if (!isPlayingNow && clock.wasPlayingLastBlock) {
            for (auto* track : trackContainer.getTracks()) {
                juce::MidiBuffer panic;
                for (int ch = 1; ch <= 16; ++ch) {
                    panic.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
                    panic.addEvent(juce::MidiMessage::controllerEvent(ch, 64, 0), 0);
                    for (int note = 0; note < 128; ++note) panic.addEvent(juce::MidiMessage::noteOff(ch, note), 0);
                }
                for (auto* p : track->plugins) if (p->isLoaded()) p->processBlock(*bufferToFill.buffer, panic);
            }
        }
        clock.wasPlayingLastBlock = isPlayingNow;

        // 2. Vista Previa MIDI
        juce::MidiBuffer previewMidi;
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

        int safeNumChannels = juce::jmax(16, bufferToFill.buffer->getNumChannels());
        int hardwareOutChannels = bufferToFill.buffer->getNumChannels();
        auto& tracks = trackContainer.getTracks();

        for (auto* track : tracks) {
            if (track->audioBuffer.getNumChannels() < safeNumChannels || track->audioBuffer.getNumSamples() < bufferToFill.numSamples) {
                track->audioBuffer.setSize(safeNumChannels, bufferToFill.numSamples);
            }
            track->audioBuffer.clear();
            track->currentPeakLevelL = 0.0f; track->currentPeakLevelR = 0.0f;
        }

        // 3. Actualizar Reloj
        clock.update(bufferToFill.numSamples, isPlayingNow, pianoRollUI.getPlayheadPos(), pianoRollUI.getLoopEndPos(), pianoRollUI.getBpm());

        // 4. Calcular Jerarquía de Carpetas (Routing)
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

        // 5. PROCESAMIENTO PISTA POR PISTA
        for (int i = (int)tracks.size() - 1; i >= 0; --i) {
            auto* track = tracks[i];
            bool isPianoRollActive = (&(track->notes) == pianoRollUI.getActiveNotesPointer());
            TrackProcessor::process(track, clock, bufferToFill.numSamples, isPlayingNow, previewMidi, isPianoRollActive, pianoRollUI.getLoopEndPos());
            MasterMixer::processTrackVolumeAndRouting(track, bufferToFill.numSamples, hardwareOutChannels, parentIndices[i], tracks, bufferToFill);
        }

        if (isPlayingNow) {
            float nPh = clock.nextPh;
            juce::MessageManager::callAsync([&pianoRollUI, &playlistUI, nPh]() {
                pianoRollUI.setPlayheadPos(nPh);
                playlistUI.setPlayheadPos(nPh);
                });
        }

        bufferToFill.buffer->applyGain(bufferToFill.startSample, bufferToFill.numSamples, mixerUI.getMasterVolume());
    }
};