#pragma once
#include <JuceHeader.h>
#include "../Core/AudioClock.h"
#include "../../Tracks/Track.h"
#include "../../UI/GainStation/GainStationDSP.h" 
#include "../../Effects/EffectsPanel.h" 
#include "../Routing/RoutingMatrix.h" // NUEVO: Para TopoState

class TrackProcessor {
public:
    static void process(Track* track,
        const AudioClock& clock,
        int numSamples,
        bool isPlayingNow,
        bool isStoppingNow,
        const juce::MidiBuffer& previewMidi,
        const RoutingMatrix::TopoState* topo) noexcept // lock-free guarantee
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
            track->postBalance.prepare(44100.0, 512);
            track->postMidSide.prepare(44100.0);
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

        // ==============================================================
        // SNAPSHOT — Adquirir la vista inmutable del audio thread.
        // Load con memory_order_acquire garantiza que vemos el snapshot
        // completo publicado por la UI mediante release/acq_rel.
        // Si el snapshot es nullptr (track recien creado), se omiten
        // clips/notas de forma segura hasta el primer commitSnapshot().
        // ==============================================================
        const TrackSnapshot* snap = track->snapshot.load(std::memory_order_acquire);

        if (isPlayingNow && snap) {
            // --- Notas directas (Piano Roll) ---
            for (const auto& note : snap->notes) {
                // Aplicamos margen de seguridad épsilon (-0.0001f) para que las notas en el inicio exacto del bloque siempre disparen
                bool triggerOn = clock.looped
                   ? ((note.x >= clock.currentPh - 0.0001f && note.x < clock.nextPh + 1.0f) || (note.x >= 0 && note.x < clock.nextPh))
                   : (note.x >= clock.currentPh - 0.0001f && note.x < clock.nextPh);

                if (triggerOn) {
                    // Calculamos offset preciso de muestra para que suene 'Professional'
                    long long noteStartSample = (long long)(note.x * clock.samplesPerPixel);
                    int offset = (int)(noteStartSample - clock.currentSamplePos);
                    if (clock.looped && noteStartSample < clock.currentSamplePos) // Caso wrap-around
                        offset = (int)(noteStartSample + (clock.loopEndSamplePos - clock.currentSamplePos));
                    
                    trackMidi.addEvent(juce::MidiMessage::noteOn(1, note.pitch, 0.8f), juce::jlimit(0, numSamples - 1, offset));
                }

                float offX = (float)(note.x + note.width);
                bool triggerOff = clock.looped
                   ? ((offX >= clock.currentPh && offX < clock.nextPh + 1.0f) || (offX >= 0 && offX < clock.nextPh))
                   : (offX >= clock.currentPh && offX < clock.nextPh);

                if (triggerOff) {
                    long long noteOffSample = (long long)(offX * clock.samplesPerPixel);
                    int offset = (int)(noteOffSample - clock.currentSamplePos);
                    if (clock.looped && noteOffSample < clock.currentSamplePos)
                        offset = (int)(noteOffSample + (clock.loopEndSamplePos - clock.currentSamplePos));
                        
                    trackMidi.addEvent(juce::MidiMessage::noteOff(1, note.pitch), juce::jlimit(0, numSamples - 1, offset));
                }
            }

            // --- MIDI clips ---
            for (const auto& clipSnap : snap->midiClips) {
                if (clipSnap.isMuted) continue;
                for (const auto& note : clipSnap.notes) {
                    // --- MOTOR MIDI RELATIVO: worldX = inicio del clip + (posición nota - offset de recorte) ---
                    float noteWorldX = clipSnap.startX + ((float)note.x - clipSnap.offsetX);
                    float offWorldX = noteWorldX + (float)note.width;

                    // Culling: solo disparamos si la nota está dentro de los límites visuales del clip
                    bool isInClip = (noteWorldX >= clipSnap.startX - 0.001f && noteWorldX < clipSnap.startX + clipSnap.width);
                    if (!isInClip) continue;

                    bool triggerOn = clock.looped
                        ? ((noteWorldX >= clock.currentPh - 0.0001f && noteWorldX < clock.nextPh + 1.0f) || (noteWorldX >= 0 && noteWorldX < clock.nextPh))
                        : (noteWorldX >= clock.currentPh - 0.0001f && noteWorldX < clock.nextPh);

                    if (triggerOn) {
                        long long nStart = (long long)(noteWorldX * clock.samplesPerPixel);
                        int offset = (int)(nStart - clock.currentSamplePos);
                        if (clock.looped && nStart < clock.currentSamplePos)
                             offset = (int)(nStart + (clock.loopEndSamplePos - clock.currentSamplePos));
                        trackMidi.addEvent(juce::MidiMessage::noteOn(1, note.pitch, 0.8f), juce::jlimit(0, numSamples - 1, offset));
                    }

                    bool triggerOff = clock.looped
                        ? ((offWorldX >= clock.currentPh && offWorldX < clock.nextPh + 1.0f) || (offWorldX >= 0 && offWorldX < clock.nextPh))
                        : (offWorldX >= clock.currentPh && offWorldX < clock.nextPh);
                    
                    if (triggerOff && offWorldX <= clipSnap.startX + clipSnap.width + 0.001f) {
                        long long nOff = (long long)(offWorldX * clock.samplesPerPixel);
                        int offset = (int)(nOff - clock.currentSamplePos);
                        if (clock.looped && nOff < clock.currentSamplePos)
                            offset = (int)(nOff + (clock.loopEndSamplePos - clock.currentSamplePos));
                        trackMidi.addEvent(juce::MidiMessage::noteOff(1, note.pitch), juce::jlimit(0, numSamples - 1, offset));
                    }
                }
            }
        }

        if (isPlayingNow && snap) {
            // --- Audio clips ---
            for (const auto& clipSnap : snap->audioClips) {
                if (clipSnap.isMuted || !clipSnap.fileBufferPtr || !clipSnap.isLoaded) continue;

                const juce::AudioBuffer<float>& fileBuf = *clipSnap.fileBufferPtr;

                long long clipStartSample = (long long)(clipSnap.startX * clock.samplesPerPixel);
                long long clipEndSample   = clipStartSample + (long long)(clipSnap.width * clock.samplesPerPixel);
                long long offsetSamples   = (long long)(clipSnap.offsetX * clock.samplesPerPixel);

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

                    if (clock.currentSamplePos + writeStart + samplesToWrite > clipEndSample) {
                        samplesToWrite = (int)(clipEndSample - (clock.currentSamplePos + writeStart));
                    }

                    int fileReadStart = readStart + (int)offsetSamples;

                    if (fileReadStart + samplesToWrite > fileBuf.getNumSamples())
                        samplesToWrite = fileBuf.getNumSamples() - fileReadStart;

                    if (writeStart + samplesToWrite > track->audioBuffer.getNumSamples())
                        samplesToWrite = track->audioBuffer.getNumSamples() - writeStart;

                    if (samplesToWrite > 0 && fileReadStart >= 0) {
                        PDCManager::dbgSamplesWritten.store(samplesToWrite, std::memory_order_relaxed);
                        PDCManager::dbgAddCount.fetch_add(1, std::memory_order_relaxed);
                        int destChannels = juce::jmin(2, track->audioBuffer.getNumChannels());
                        for (int ch = 0; ch < destChannels; ++ch) {
                            int sourceCh = juce::jmin(ch, fileBuf.getNumChannels() - 1);
                            if (sourceCh >= 0) {
                                track->audioBuffer.addFrom(ch, writeStart, fileBuf, sourceCh, fileReadStart, samplesToWrite);
                            }
                        }
                    }
                }
            }
        }

        juce::AudioBuffer<float> mainProxyBuffer(track->audioBuffer.getArrayOfWritePointers(), track->audioBuffer.getNumChannels(), numSamples);

        int numChannels = track->audioBuffer.getNumChannels();
        int safeChannels = juce::jmin(numChannels, 2);

        // Ya no usamos setSize(...) lo que detendría el realloc en tiempo real
        // Requerimos que prepareToPlay de MainComponent pre-aloje instrumentMixBuffer y tempSynthBuffer a 2 canales
        track->instrumentMixBuffer.clear();
        bool hasInstruments = false;

        for (auto* p : track->plugins) {
            if (p != nullptr && p->isLoaded() && p->getIsInstrument()) {
                hasInstruments = true;
                track->tempSynthBuffer.clear();

                juce::MidiBuffer midiForThisSynth;
                midiForThisSynth.ensureSize(2048);
                midiForThisSynth.addEvents(trackMidi, 0, numSamples, 0);

                p->updatePlayHead(isPlayingNow, clock.currentSamplePos);
                
                // Truco de encapsulado de buffer temporal para evitar setSize
                juce::AudioBuffer<float> proxyBuffer(track->tempSynthBuffer.getArrayOfWritePointers(), safeChannels, numSamples);
                
                // --- SIDECHAIN SUPPORT ---
                const juce::AudioBuffer<float>* sidechainBuf = nullptr;
                int scId = p->sidechainSourceTrackId.load(std::memory_order_relaxed);
                if (scId != -1 && topo != nullptr) {
                    for (int i = 0; i < topo->activeTracks.size(); ++i) {
                        auto* t = topo->activeTracks[i];
                        if (t->getId() == scId) {
                            sidechainBuf = &t->audioBuffer;
                            break;
                        }
                    }
                }
                
                p->processBlock(proxyBuffer, midiForThisSynth, sidechainBuf);

                for (int ch = 0; ch < safeChannels; ++ch) {
                    track->instrumentMixBuffer.addFrom(ch, 0, track->tempSynthBuffer, ch, 0, numSamples);
                }
            }
        }

        // --- FORZAR SUMA SI HAY INSTRUMENTO (Incluso si no hay clips) ---
        if (hasInstruments) {
            for (int ch = 0; ch < safeChannels; ++ch) {
                track->audioBuffer.addFrom(ch, 0, track->instrumentMixBuffer, ch, 0, numSamples);
            }
        }

        // --- CHANNEL EQ (INLINE SOUND DESIGN) ---
        // Se aplica inmediatamente como moldeado base del track (Audio + Sintetizador)
        {
            juce::dsp::AudioBlock<float> eqBlock(mainProxyBuffer);
            track->inlineEQ.process(eqBlock);
        }

        // --- PRE-FX & GAIN STATION ---
        // Medimos el sonido 'crudo' ecualizado antes de enviarlo a los plugins de inserción.
        GainStationDSP::processPreFX(track, mainProxyBuffer);

        for (auto* p : track->plugins) {
            if (p != nullptr && p->isLoaded() && !p->getIsInstrument()) {
                PluginRouting r = p->getRouting();

                p->updatePlayHead(isPlayingNow, clock.currentSamplePos);

                // --- SIDECHAIN SUPPORT ---
                const juce::AudioBuffer<float>* sidechainBuf = nullptr;
                int scId = p->sidechainSourceTrackId.load(std::memory_order_relaxed);
                if (scId != -1 && topo != nullptr) {
                    for (int i = 0; i < topo->activeTracks.size(); ++i) {
                        auto* t = topo->activeTracks[i];
                        if (t->getId() == scId) {
                            sidechainBuf = &t->audioBuffer;
                            break;
                        }
                    }
                }

                if (r == PluginRouting::Stereo || numChannels < 2) {
                    juce::AudioBuffer<float> proxyBuffer(track->audioBuffer.getArrayOfWritePointers(), numChannels, numSamples);
                    p->processBlock(proxyBuffer, trackMidi, sidechainBuf);
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
                    p->processBlock(proxyBuffer, trackMidi, sidechainBuf);

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

        // --- POST-FX & GAIN STATION ---
        // Usamos mainProxyBuffer para medir unicamente los samples validos de este bloque.
        GainStationDSP::processPostFX(track, mainProxyBuffer);

        if (numChannels > 2) {
            for (int ch = 2; ch < numChannels; ++ch) {
                track->audioBuffer.clear(ch, 0, numSamples);
            }
        }
    }
};
