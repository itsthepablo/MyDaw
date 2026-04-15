#include "TrackProcessor.h"
#include "../../Modules/GainStation/DSP/GainStationDSP.h"
#include "../../Modules/LoudnessTrack/DSP/LoudnessTrackDSP.h"
#include "../../Modules/BalanceTrack/DSP/BalanceTrackDSP.h"
#include "../../Modules/MidSideTrack/DSP/MidSideTrackDSP.h"
#include "../Modulation/NativeModulationManager.h"

void TrackProcessor::process(Track* track,
    const AudioClock& clock,
    int numSamples,
    bool isPlayingNow,
    bool isStoppingNow,
    const juce::MidiBuffer& previewMidi,
    const RoutingMatrix::TopoState* topo) noexcept
{
    bool hasClipsOrNotes = !(track->getAudioClips().isEmpty() && track->getMidiClips().isEmpty() && track->notes.empty());

    PDCManager::dbgClips.store(track->getAudioClips().size(), std::memory_order_relaxed);

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

    if (!track->dsp.isAnalyzersPrepared) {
        track->dsp.preLoudness.prepare(44100.0, 512);
        track->dsp.postLoudness.prepare(44100.0, 512);
        track->dsp.postBalance.prepare(44100.0, 512);
        track->dsp.postMidSide.prepare(44100.0);
        track->dsp.isAnalyzersPrepared = true;
    }

    juce::MidiBuffer trackMidi;
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

    const TrackSnapshot* snap = track->snapshot.load(std::memory_order_acquire);

    if ((isPlayingNow || isStoppingNow) && snap) {
        // --- Notas directas (Piano Roll) ---
        for (const auto& note : snap->notes) {
            bool triggerOn = clock.looped
                ? ((note.x >= clock.currentPh - 0.0001f && note.x < clock.nextPh + 1.0f) || (note.x >= 0 && note.x < clock.nextPh))
                : (note.x >= clock.currentPh - 0.0001f && note.x < clock.nextPh);

            if (triggerOn) {
                long long noteStartSample = (long long)(note.x * clock.samplesPerPixel);
                int offset = (int)(noteStartSample - clock.currentSamplePos);
                if (clock.looped && noteStartSample < clock.currentSamplePos) 
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
                float noteWorldX = clipSnap.startX + ((float)note.x - clipSnap.offsetX);
                float offWorldX = noteWorldX + (float)note.width;

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

    if ((isPlayingNow || isStoppingNow) && snap) {
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

    track->instrumentMixBuffer.clear();
    bool hasInstruments = false;

    for (auto* p : track->plugins) {
        if (p != nullptr && p->isLoaded() && p->getIsInstrument()) {
            hasInstruments = true;
            track->tempSynthBuffer.clear();

            juce::MidiBuffer midiForThisSynth;
            midiForThisSynth.ensureSize(2048);
            midiForThisSynth.addEvents(trackMidi, 0, numSamples, 0);

            p->updatePlayHead(isPlayingNow || isStoppingNow, clock.currentSamplePos);
            
            juce::AudioBuffer<float> proxyBuffer(track->tempSynthBuffer.getArrayOfWritePointers(), safeChannels, numSamples);
            
            const juce::AudioBuffer<float>* sidechainBuf = nullptr;
            int scId = p->sidechain.sourceTrackId.load(std::memory_order_relaxed);
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

    if (hasInstruments) {
        for (int ch = 0; ch < safeChannels; ++ch) {
            track->audioBuffer.addFrom(ch, 0, track->instrumentMixBuffer, ch, 0, numSamples);
        }
    }

    {
        juce::dsp::AudioBlock<float> eqBlock(mainProxyBuffer);
        track->dsp.inlineEQ.process(eqBlock);
    }

    GainStationDSP::processPreFX(track->gainStationData, track, mainProxyBuffer);
    
    // --- DESPACHADOR DE MODULACIÓN UNIVERSAL NATIVA (PROFESSIONAL) ---
    // Este gestor centraliza la modulación de todos los parámetros nativos mapeados via LEARN.
    NativeModulationManager modManager;
    modManager.applyModulations(track, clock.currentPh);

    // --- APLICAR MODULACIÓN A PLUGINS (THROTTLED & AGGREGATED) ---
    for (auto* m : track->modulators) {
        juce::ScopedLock sl(m->targetsLock);
        for (const auto& target : m->targets) {
            if (target.type == ModTarget::PluginParam && target.pluginIdx >= 0 && target.pluginIdx < track->plugins.size()) {
                if (auto* p = track->plugins[target.pluginIdx]) {
                    float val = m->getValueAt(clock.currentPh);
                    p->setParameterModulation(target.parameterIdx, val);
                }
            }
        }
    }

    // Finalizar agregación y aplicar valores reales a los plugins
    for (auto* p : track->plugins) {
        if (p != nullptr) p->applyModulations();
    }

    for (auto* p : track->plugins) {
        if (p != nullptr && p->isLoaded() && !p->getIsInstrument()) {
            PluginRouting r = p->getRouting();

            p->updatePlayHead(isPlayingNow || isStoppingNow, clock.currentSamplePos);

            const juce::AudioBuffer<float>* sidechainBuf = nullptr;
            int scId = p->sidechain.sourceTrackId.load(std::memory_order_relaxed);
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

    GainStationDSP::processPostFX(track->gainStationData, track, mainProxyBuffer);

    if (isPlayingNow && track->isLoudnessRecording) {
        LoudnessTrackDSP::recordAnalysis(track->loudnessTrackData, track->dsp.postLoudness, clock.currentSamplePos);
        BalanceTrackDSP::recordAnalysis(track->balanceTrackData, track->dsp.postBalance, clock.currentSamplePos);
        MidSideTrackDSP::recordAnalysis(track->midSideTrackData, track->dsp.postMidSide, clock.currentSamplePos);
    }

    if (numChannels > 2) {
        for (int ch = 2; ch < numChannels; ++ch) {
            track->audioBuffer.clear(ch, 0, numSamples);
        }
    }
}
