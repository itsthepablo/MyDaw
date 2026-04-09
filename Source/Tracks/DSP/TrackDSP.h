#pragma once
#include <JuceHeader.h>
#include "../Data/Track.h"

/**
 * TrackDSP: Encapsula la lógica de procesamiento de audio específica de un track.
 * Siguiendo el patrón de MixerDSP, esta clase procesará el audioBuffer del track.
 */
class TrackDSP {
public:
    TrackDSP() {}

    /**
     * Prepara el procesador para una nueva frecuencia de muestreo.
     */
    void prepare(double sampleRate, int samplesPerBlock) {
        ignoreUnused(sampleRate, samplesPerBlock);
    }

    /**
     * Procesa un bloque de audio del track.
     */
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
        ignoreUnused(buffer, midiMessages);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackDSP)
};
