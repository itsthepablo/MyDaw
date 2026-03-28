#pragma once
#include <JuceHeader.h>
#include "AudioClock.h"
#include "../Tracks/Track.h"
#include "../UI/GainStation/GainStationDSP.h" 
#include "../Effects/EffectsPanel.h" 

class TrackProcessor {
public:
    static void process(Track* track,
        const AudioClock& clock,
        int numSamples,
        bool isPlayingNow,
        bool isStoppingNow, // <-- NUEVA VARIABLE
        const juce::MidiBuffer& previewMidi,
        bool isPianoRollActive,
        float loopEndPos)
    {
        bool hasClipsOrNotes = !(track->audioClips.isEmpty() && track->midiClips.isEmpty() && track->notes.empty());

        float magL = track->audioBuffer.getMagnitude(0, 0, numSamples);
        float magR = track->audioBuffer.getNumChannels() > 1 ? track->audioBuffer.getMagnitude(1, 0, numSamples) : 0.0f;
        bool isSilent = (magL < 0.00001f && magR < 0.00001f);

        bool hasPlugins = false;
        for (auto* p : track->plugins) if (p->isLoaded()) { hasPlugins = true; break; }

        // Si estamos enviando el Pánico (isStoppingNow), prohibimos el Smart Disable
        if (!hasClipsOrNotes && !isPianoRollActive && isSilent && !hasPlugins && !isStoppingNow) {
            track->audioBuffer.clear();
            return;
        }

        if (!track->isAnalyzersPrepared) {
            track->preLoudness.prepare(44100.0, 512);
            track->postLoudness.prepare(44100.0, 512);
            track->isAnalyzersPrepared = true;
        }

        juce::MidiBuffer trackMidi;
        trackMidi.ensureSize(2048);

        // ==============================================================================
        // INYECCIÓN QUIRÚRGICA DE PÁNICO
        // Si acabamos de darle a Stop, apagamos los sintes aquí adentro, de forma
        // perfectamente sincronizada con el PDC.
        // ==============================================================================
        if (isStoppingNow) {
            for (int ch = 1; ch <= 16; ++ch) {
                trackMidi.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
                trackMidi.addEvent(juce::MidiMessage::controllerEvent(ch, 64, 0), 0); // Apagar Pedal Sustain
                for (int note = 0; note < 128; ++note) {
                    trackMidi.addEvent(juce::MidiMessage::noteOff(ch, note), 0);
                }
            }
        }

        if (isPianoRollActive) trackMidi.addEvents(previewMidi, 0, numSamples, 0);

        if (isPlayingNow) {
            for (const auto& note : track->notes) {
                bool triggerOn = clock.looped ? ((note.x >= clock.currentPh && note.x < loopEndPos) || (note.x >= 0 && note.x < clock.nextPh)) : (note.x >= clock.currentPh && note.x < clock.nextPh);
                if (triggerOn) trackMidi.addEvent(juce::MidiMessage::noteOn(1, note.pitch, 0.8f), 0);

                float offX = note.x + note.width;
                bool triggerOff = clock.looped ? ((offX >= clock.currentPh && offX < loopEndPos) || (offX >= 0 && offX < clock.nextPh)) : (offX >= clock.currentPh && offX < clock.nextPh);
                if (triggerOff) trackMidi.addEvent(juce::MidiMessage::noteOff(1, note.pitch), 0);
            }

            for (auto* clip : track->midiClips) {
                for (const auto& note : clip->notes) {
                    bool triggerOn = clock.looped ? ((note.x >= clock.currentPh && note.x < loopEndPos) || (note.x >= 0 && note.x < clock.nextPh)) : (note.x >= clock.currentPh && note.x < clock.nextPh);
                    if (triggerOn) trackMidi.addEvent(juce::MidiMessage::noteOn(1, note.pitch, 0.8f), 0);

                    float offX = note.x + note.width;
                    bool triggerOff = clock.looped ? ((offX >= clock.currentPh && offX < loopEndPos) || (offX >= 0 && offX < clock.nextPh)) : (offX >= clock.currentPh && offX < clock.nextPh);
                    if (triggerOff) trackMidi.addEvent(juce::MidiMessage::noteOff(1, note.pitch), 0);
                }
            }
        }

        if (isPlayingNow) {
            for (auto* clip : track->audioClips) {
                long long clipStartSample = (long long)(clip->startX * clock.samplesPerPixel);
                long long clipEndSample = clipStartSample + clip->fileBuffer.getNumSamples();

                if (clipStartSample < clock.blockEndSamplePos && clipEndSample > clock.currentSamplePos) {
                    int readStart = 0;
                    int writeStart = 0;
                    int samplesToWrite = numSamples;

                    if (clipStartSample > clock.currentSamplePos) {
                        writeStart = (int)(clipStartSample - clock.currentSamplePos);
                        samplesToWrite -= writeStart;
                    }
                    else {
                        readStart = (int)(clock.currentSamplePos - clipStartSample);
                    }

                    if (readStart + samplesToWrite > clip->fileBuffer.getNumSamples())
                        samplesToWrite = clip->fileBuffer.getNumSamples() - readStart;

                    if (samplesToWrite > 0) {
                        int destChannels = juce::jmin(2, track->audioBuffer.getNumChannels());
                        for (int ch = 0; ch < destChannels; ++ch) {
                            int sourceCh = juce::jmin(ch, clip->fileBuffer.getNumChannels() - 1);
                            track->audioBuffer.addFrom(ch, writeStart, clip->fileBuffer, sourceCh, readStart, samplesToWrite);
                        }
                    }
                }
            }
        }

        GainStationDSP::processPreFX(track, track->audioBuffer);

        int numChannels = track->audioBuffer.getNumChannels();
        int safeChannels = juce::jmin(numChannels, 2);

        track->instrumentMixBuffer.setSize(safeChannels, numSamples, false, false, true);
        track->tempSynthBuffer.setSize(safeChannels, numSamples, false, false, true);

        track->instrumentMixBuffer.clear();
        bool hasInstruments = false;

        for (auto* p : track->plugins) {
            if (p->isLoaded() && EffectsPanel::pluginIsInstrumentMap[(void*)p]) {
                hasInstruments = true;
                track->tempSynthBuffer.clear();

                juce::MidiBuffer midiForThisSynth;
                midiForThisSynth.ensureSize(2048);
                midiForThisSynth.addEvents(trackMidi, 0, numSamples, 0);

                p->processBlock(track->tempSynthBuffer, midiForThisSynth);

                for (int ch = 0; ch < safeChannels; ++ch) {
                    track->instrumentMixBuffer.addFrom(ch, 0, track->tempSynthBuffer, ch, 0, numSamples);
                }
            }
        }

        if (hasInstruments) {
            for (int ch = 0; ch < safeChannels; ++ch) {
                track->audioBuffer.addFrom(ch, 0, track->instrumentMixBuffer, ch, 0, numSamples);
            }
        }

        for (auto* p : track->plugins) {
            if (p->isLoaded() && !EffectsPanel::pluginIsInstrumentMap[(void*)p]) {
                PluginRouting r = p->getRouting();

                if (r == PluginRouting::Stereo || numChannels < 2) {
                    p->processBlock(track->audioBuffer, trackMidi);
                }
                else {
                    track->midSideBuffer.setSize(1, numSamples, false, false, true);
                    auto* preservedSignal = track->midSideBuffer.getWritePointer(0);

                    auto* left = track->audioBuffer.getWritePointer(0);
                    auto* right = track->audioBuffer.getWritePointer(1);

                    for (int i = 0; i < numSamples; ++i) {
                        float mid = (left[i] + right[i]) * 0.5f;
                        float side = (left[i] - right[i]) * 0.5f;

                        if (r == PluginRouting::Mid) {
                            preservedSignal[i] = side;
                            left[i] = mid;
                            right[i] = mid;
                        }
                        else if (r == PluginRouting::Side) {
                            preservedSignal[i] = mid;
                            left[i] = side;
                            right[i] = side;
                        }
                    }

                    p->processBlock(track->audioBuffer, trackMidi);

                    for (int i = 0; i < numSamples; ++i) {
                        float processedMono = (left[i] + right[i]) * 0.5f;

                        if (r == PluginRouting::Mid) {
                            left[i] = processedMono + preservedSignal[i];
                            right[i] = processedMono - preservedSignal[i];
                        }
                        else if (r == PluginRouting::Side) {
                            left[i] = preservedSignal[i] + processedMono;
                            right[i] = preservedSignal[i] - processedMono;
                        }
                    }
                }
            }
        }

        GainStationDSP::processPostFX(track, track->audioBuffer);

        if (numChannels > 2) {
            for (int ch = 2; ch < numChannels; ++ch) {
                track->audioBuffer.clear(ch, 0, numSamples);
            }
        }
    }
};