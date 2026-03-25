#pragma once
#include <JuceHeader.h>
#include "AudioClock.h"
#include "../Tracks/Track.h"
#include "../UI/GainStation/GainStationDSP.h" // Importación del Motor DSP modular

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
        // PROCESAMIENTO DE PLUGINS CON MATRIZ MID/SIDE NATIVA (ZERO-ALLOCATION)
        // ==============================================================================
        int numChannels = track->audioBuffer.getNumChannels();
        int numSamplesBlock = track->audioBuffer.getNumSamples();

        for (auto* p : track->plugins) {
            if (p->isLoaded()) {
                PluginRouting r = p->getRouting();

                if (r == PluginRouting::Stereo || numChannels < 2) {
                    // Flujo Estéreo Normal
                    p->processBlock(track->audioBuffer, trackMidi);
                }
                else {
                    // MATRIZ M/S NATIVA
                    // Usamos un buffer de pila en lugar de alojar memoria dinámica para no romper el hilo de audio en tiempo real
                    float preservedSignal[4096];
                    int samplesToProcess = juce::jmin(numSamplesBlock, 4096);

                    auto* left = track->audioBuffer.getWritePointer(0);
                    auto* right = track->audioBuffer.getWritePointer(1);

                    // 1. CODIFICAR: Separar M/S y alimentar al plugin solo la señal elegida
                    for (int i = 0; i < samplesToProcess; ++i) {
                        float mid = (left[i] + right[i]) * 0.5f;
                        float side = (left[i] - right[i]) * 0.5f;

                        if (r == PluginRouting::Mid) {
                            preservedSignal[i] = side; // Salvamos el Side para no alterarlo
                            left[i] = mid;
                            right[i] = mid;            // Alimentamos Mid puro a ambos canales del VST
                        }
                        else if (r == PluginRouting::Side) {
                            preservedSignal[i] = mid;  // Salvamos el Mid para no alterarlo
                            left[i] = side;
                            right[i] = side;           // Alimentamos Side puro a ambos canales del VST
                        }
                    }

                    // 2. PROCESAR: El VST ahora solo está procesando el centro (Mid) o los bordes (Side)
                    p->processBlock(track->audioBuffer, trackMidi);

                    // 3. DECODIFICAR: Mezclar la señal tratada por el VST con la que mantuvimos intacta
                    for (int i = 0; i < samplesToProcess; ++i) {
                        // Promediamos L y R del VST para evitar artefactos si el usuario paneó dentro del plugin
                        float processedMono = (left[i] + right[i]) * 0.5f;

                        if (r == PluginRouting::Mid) {
                            // Reconstruimos el estéreo: (Nuevo Mid) + (Side Original)
                            left[i] = processedMono + preservedSignal[i];
                            right[i] = processedMono - preservedSignal[i];
                        }
                        else if (r == PluginRouting::Side) {
                            // Reconstruimos el estéreo: (Mid Original) + (Nuevo Side)
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