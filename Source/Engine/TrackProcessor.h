#pragma once
#include <JuceHeader.h>
#include "AudioClock.h"

class TrackProcessor {
public:
    static void process(Track* track, 
                        const AudioClock& clock, 
                        int numSamples, 
                        bool isPlayingNow, 
                        juce::MidiBuffer& previewMidi, 
                        bool isPianoRollActive,
                        float loopEndPos) 
    {
        juce::MidiBuffer trackMidi;

        // 1. Leer MIDI del Piano Roll
        if (isPianoRollActive) {
            trackMidi.addEvents(previewMidi, 0, numSamples, 0);
            if (isPlayingNow) {
                for (const auto& note : track->notes) {
                    bool triggerOn = clock.looped ? ((note.x >= clock.currentPh && note.x < loopEndPos) || (note.x >= 0 && note.x < clock.nextPh)) : (note.x >= clock.currentPh && note.x < clock.nextPh);
                    if (triggerOn) trackMidi.addEvent(juce::MidiMessage::noteOn(1, note.pitch, 0.8f), 0);
                    
                    float offX = note.x + note.width;
                    bool triggerOff = clock.looped ? ((offX >= clock.currentPh && offX < loopEndPos) || (offX >= 0 && offX < clock.nextPh)) : (offX >= clock.currentPh && offX < clock.nextPh);
                    if (triggerOff) trackMidi.addEvent(juce::MidiMessage::noteOff(1, note.pitch), 0);
                }
            }
        }

        // 2. Leer Clips de Audio
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
                    } else {
                        readStart = (int)(clock.currentSamplePos - clipStartSample);
                    }

                    if (readStart + samplesToWrite > clip->fileBuffer.getNumSamples()) {
                        samplesToWrite = clip->fileBuffer.getNumSamples() - readStart;
                    }

                    if (samplesToWrite > 0) {
                        for (int ch = 0; ch < track->audioBuffer.getNumChannels(); ++ch) {
                            int sourceCh = juce::jmin(ch, clip->fileBuffer.getNumChannels() - 1);
                            track->audioBuffer.addFrom(ch, writeStart, clip->fileBuffer, sourceCh, readStart, samplesToWrite);
                        }
                    }
                }
            }
        }

        // 3. Procesar Plugins VST
        for (auto* p : track->plugins) {
            if (p->isLoaded()) p->processBlock(track->audioBuffer, trackMidi);
        }
    }
};