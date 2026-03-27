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
        const juce::MidiBuffer& previewMidi,
        bool isPianoRollActive,
        float loopEndPos)
    {
        if (!track->isAnalyzersPrepared) {
            track->preLoudness.prepare(44100.0, 512);
            track->postLoudness.prepare(44100.0, 512);
            track->isAnalyzersPrepared = true;
        }

        juce::MidiBuffer trackMidi;

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
                        for (int ch = 0; ch < track->audioBuffer.getNumChannels(); ++ch) {
                            int sourceCh = juce::jmin(ch, clip->fileBuffer.getNumChannels() - 1);
                            track->audioBuffer.addFrom(ch, writeStart, clip->fileBuffer, sourceCh, readStart, samplesToWrite);
                        }
                    }
                }
            }
        }

        // ==============================================================================
        // GAIN STATION: PRE-FX
        // ==============================================================================
        GainStationDSP::processPreFX(track, track->audioBuffer);

        // ==============================================================================
        // 1. LAYERING: PROCESAMIENTO DE INSTRUMENTOS (PARALELO)
        // ==============================================================================
        int numChannels = track->audioBuffer.getNumChannels();
        int numSamplesBlock = track->audioBuffer.getNumSamples();

        float mixL[4096] = { 0 }; float mixR[4096] = { 0 };
        float tempL[4096] = { 0 }; float tempR[4096] = { 0 };

        int safeSamples = juce::jmin(numSamplesBlock, 4096);
        int safeChannels = juce::jmin(numChannels, 2);

        float* mixPointers[] = { mixL, mixR };
        float* tempPointers[] = { tempL, tempR };

        juce::AudioBuffer<float> instrumentMixBuffer(mixPointers, safeChannels, safeSamples);
        juce::AudioBuffer<float> tempSynthBuffer(tempPointers, safeChannels, safeSamples);

        instrumentMixBuffer.clear();
        bool hasInstruments = false;

        for (auto* p : track->plugins) {
            if (p->isLoaded() && EffectsPanel::pluginIsInstrumentMap[(void*)p]) {
                hasInstruments = true;
                tempSynthBuffer.clear();

                // --- CORRECCIÓN CRÍTICA: CLONAR EL MIDI BUFFER ---
                // Le entregamos a cada instrumento una copia fresca de los eventos MIDI
                // para evitar que el primero consuma los eventos y silencie al resto.
                juce::MidiBuffer midiForThisSynth;
                midiForThisSynth.addEvents(trackMidi, 0, numSamplesBlock, 0);

                p->processBlock(tempSynthBuffer, midiForThisSynth);

                for (int ch = 0; ch < safeChannels; ++ch) {
                    instrumentMixBuffer.addFrom(ch, 0, tempSynthBuffer, ch, 0, safeSamples);
                }
            }
        }

        if (hasInstruments) {
            for (int ch = 0; ch < safeChannels; ++ch) {
                track->audioBuffer.addFrom(ch, 0, instrumentMixBuffer, ch, 0, safeSamples);
            }
        }

        // ==============================================================================
        // 2. EFECTOS: PROCESAMIENTO EN SERIE (CON MATRIZ MID/SIDE)
        // ==============================================================================
        for (auto* p : track->plugins) {
            if (p->isLoaded() && !EffectsPanel::pluginIsInstrumentMap[(void*)p]) {
                PluginRouting r = p->getRouting();

                if (r == PluginRouting::Stereo || numChannels < 2) {
                    p->processBlock(track->audioBuffer, trackMidi);
                }
                else {
                    float preservedSignal[4096];
                    int samplesToProcess = juce::jmin(numSamplesBlock, 4096);

                    auto* left = track->audioBuffer.getWritePointer(0);
                    auto* right = track->audioBuffer.getWritePointer(1);

                    for (int i = 0; i < samplesToProcess; ++i) {
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

                    for (int i = 0; i < samplesToProcess; ++i) {
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

        // ==============================================================================
        // GAIN STATION: POST-FX
        // ==============================================================================
        GainStationDSP::processPostFX(track, track->audioBuffer);
    }
};