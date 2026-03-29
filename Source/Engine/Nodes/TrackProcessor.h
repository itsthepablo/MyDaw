#pragma once
#include <JuceHeader.h>
#include "../Core/AudioClock.h"
#include "../../Tracks/Track.h"
#include "../../UI/GainStation/GainStationDSP.h" 
#include "../../Effects/EffectsPanel.h" 

class TrackProcessor {
public:
    static void process(Track* track,
        const AudioClock& clock,
        int numSamples,
        bool isPlayingNow,
        bool isStoppingNow,
        const juce::MidiBuffer& previewMidi) noexcept // lock-free guarantee
    {
        bool hasClipsOrNotes = !(track->audioClips.isEmpty() && track->midiClips.isEmpty() && track->notes.empty());

        PDCManager::dbgClips.store(track->audioClips.size(), std::memory_order_relaxed);

        float magL = track->audioBuffer.getMagnitude(0, 0, numSamples);
        float magR = track->audioBuffer.getNumChannels() > 1 ? track->audioBuffer.getMagnitude(1, 0, numSamples) : 0.0f;
        bool isSilent = (magL < 0.00001f && magR < 0.00001f);

        bool hasPlugins = false;
        for (auto* p : track->plugins) if (p != nullptr && p->isLoaded()) { hasPlugins = true; break; }

        bool isSelected = track->isSelected;

        if (!hasClipsOrNotes && !isSelected && isSilent && !hasPlugins && !isStoppingNow) {
            track->audioBuffer.clear();
            return;
        }

        if (!track->isAnalyzersPrepared) {
            track->preLoudness.prepare(44100.0, 512);
            track->postLoudness.prepare(44100.0, 512);
            track->isAnalyzersPrepared = true;
        }

        juce::MidiBuffer trackMidi;
        // Asumimos que ensureSize no re-alofa si ya tiene memoria, pero en rigor real-time, debería estar pre-alocado
        trackMidi.ensureSize(2048);

        if (isStoppingNow) {
            for (int ch = 1; ch <= 16; ++ch) {
                trackMidi.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
                trackMidi.addEvent(juce::MidiMessage::controllerEvent(ch, 64, 0), 0);
                for (int note = 0; note < 128; ++note) {
                    trackMidi.addEvent(juce::MidiMessage::noteOff(ch, note), 0);
                }
            }
        }

        if (isSelected) trackMidi.addEvents(previewMidi, 0, numSamples, 0);

        if (isPlayingNow) {
            // Nota: Este bucle asume que 'notes' y 'midiClips' no mutan desde la UI mientras procesamos.
            // La solución 100% estricta requeriría iterar sobre un clon atómico (AudioNodeData).
            for (const auto& note : track->notes) {
                bool triggerOn = clock.looped ? ((note.x >= clock.currentPh && note.x < clock.nextPh + clock.looped) || (note.x >= 0 && note.x < clock.nextPh)) : (note.x >= clock.currentPh && note.x < clock.nextPh);
                if (triggerOn) trackMidi.addEvent(juce::MidiMessage::noteOn(1, note.pitch, 0.8f), 0);

                float offX = note.x + note.width;
                bool triggerOff = clock.looped ? ((offX >= clock.currentPh && offX < clock.nextPh + clock.looped) || (offX >= 0 && offX < clock.nextPh)) : (offX >= clock.currentPh && offX < clock.nextPh);
                if (triggerOff) trackMidi.addEvent(juce::MidiMessage::noteOff(1, note.pitch), 0);
            }

            for (auto* clip : track->midiClips) {
                if (clip) {
                    for (const auto& note : clip->notes) {
                        bool triggerOn = clock.looped ? ((note.x >= clock.currentPh && note.x < clock.nextPh + clock.looped) || (note.x >= 0 && note.x < clock.nextPh)) : (note.x >= clock.currentPh && note.x < clock.nextPh);
                        if (triggerOn) trackMidi.addEvent(juce::MidiMessage::noteOn(1, note.pitch, 0.8f), 0);

                        float offX = note.x + note.width;
                        bool triggerOff = clock.looped ? ((offX >= clock.currentPh && offX < clock.nextPh + clock.looped) || (offX >= 0 && offX < clock.nextPh)) : (offX >= clock.currentPh && offX < clock.nextPh);
                        if (triggerOff) trackMidi.addEvent(juce::MidiMessage::noteOff(1, note.pitch), 0);
                    }
                }
            }
        }

        if (isPlayingNow) {
            for (auto* clip : track->audioClips) {
                if (!clip) continue;
                long long clipStartSample = (long long)(clip->startX * clock.samplesPerPixel);
                long long clipEndSample = clipStartSample + clip->fileBuffer.getNumSamples();

                if (clipStartSample < clock.blockEndSamplePos && clipEndSample > clock.currentSamplePos) {
                    int readStart = 0;
                    int writeStart = 0;
                    int samplesToWrite = numSamples;

                    if (clipStartSample > clock.currentSamplePos) {
                        writeStart = (int)(clipStartSample - clock.currentSamplePos);
                        samplesToWrite -= writeStart;
                    } else {
                        readStart = (int)(clock.currentSamplePos - clipStartSample);
                    }

                    if (readStart + samplesToWrite > clip->fileBuffer.getNumSamples())
                        samplesToWrite = clip->fileBuffer.getNumSamples() - readStart;

                    if (writeStart + samplesToWrite > track->audioBuffer.getNumSamples())
                        samplesToWrite = track->audioBuffer.getNumSamples() - writeStart;

                    if (samplesToWrite > 0) {
                        PDCManager::dbgSamplesWritten.store(samplesToWrite, std::memory_order_relaxed);
                        PDCManager::dbgAddCount.fetch_add(1, std::memory_order_relaxed);
                        int destChannels = juce::jmin(2, track->audioBuffer.getNumChannels());
                        for (int ch = 0; ch < destChannels; ++ch) {
                            int sourceCh = juce::jmin(ch, clip->fileBuffer.getNumChannels() - 1);
                            if (sourceCh >= 0) {
                                track->audioBuffer.addFrom(ch, writeStart, clip->fileBuffer, sourceCh, readStart, samplesToWrite);
                            }
                        }
                    }
                }
            }
        }

        GainStationDSP::processPreFX(track, track->audioBuffer);

        int numChannels = track->audioBuffer.getNumChannels();
        int safeChannels = juce::jmin(numChannels, 2);

        // Ya no usamos setSize(...) lo que detendría el realloc en tiempo real
        // Requerimos que prepareToPlay de MainComponent pre-aloje instrumentMixBuffer y tempSynthBuffer a 2 canales
        track->instrumentMixBuffer.clear();
        bool hasInstruments = false;

        for (auto* p : track->plugins) {
            if (p != nullptr && p->isLoaded() && EffectsPanel::pluginIsInstrumentMap[(void*)p]) {
                hasInstruments = true;
                track->tempSynthBuffer.clear();

                juce::MidiBuffer midiForThisSynth;
                midiForThisSynth.ensureSize(2048);
                midiForThisSynth.addEvents(trackMidi, 0, numSamples, 0);

                p->updatePlayHead(isPlayingNow, clock.currentSamplePos);
                
                // Truco de encapsulado de buffer temporal para evitar setSize
                juce::AudioBuffer<float> proxyBuffer(track->tempSynthBuffer.getArrayOfWritePointers(), safeChannels, numSamples);
                p->processBlock(proxyBuffer, midiForThisSynth);

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
            if (p != nullptr && p->isLoaded() && !EffectsPanel::pluginIsInstrumentMap[(void*)p]) {
                PluginRouting r = p->getRouting();

                p->updatePlayHead(isPlayingNow, clock.currentSamplePos);

                if (r == PluginRouting::Stereo || numChannels < 2) {
                    juce::AudioBuffer<float> proxyBuffer(track->audioBuffer.getArrayOfWritePointers(), numChannels, numSamples);
                    p->processBlock(proxyBuffer, trackMidi);
                } else {
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
                        } else if (r == PluginRouting::Side) {
                            preservedSignal[i] = mid;
                            left[i] = side;
                            right[i] = side;
                        }
                    }

                    juce::AudioBuffer<float> proxyBuffer(track->audioBuffer.getArrayOfWritePointers(), numChannels, numSamples);
                    p->processBlock(proxyBuffer, trackMidi);

                    for (int i = 0; i < numSamples; ++i) {
                        float processedMono = (left[i] + right[i]) * 0.5f;

                        if (r == PluginRouting::Mid) {
                            left[i] = processedMono + preservedSignal[i];
                            right[i] = processedMono - preservedSignal[i];
                        } else if (r == PluginRouting::Side) {
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
