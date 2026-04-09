#pragma once
#include <JuceHeader.h>
#include "../../Data/Track.h"

class MasterMixer {
public:
    // Los métodos de procesamiento (Volumen/Pan) se han movido a MixerDSP.h

    // ============================================================
    // Enruta el audio del track al output maestro (bufferToFill).
    // Llamar exclusivamente desde el audio thread principal (Fase 2),
    // NUNCA desde workers paralelos — bufferToFill es compartido.
    // ============================================================
    static void routeToMaster(Track* track, int numSamples, int hardwareOutChannels,
                               const juce::AudioSourceChannelInfo& bufferToFill) noexcept
    {
        int channelsToMix = juce::jmin(hardwareOutChannels, track->audioBuffer.getNumChannels());
        for (int c = 0; c < channelsToMix; ++c)
            bufferToFill.buffer->addFrom(c, bufferToFill.startSample,
                                         track->audioBuffer, c, 0, numSamples);
    }

    // ============================================================
    // Enruta el audio resumido de un track a otro track (ej. al MASTER).
    // ============================================================
    static void routeToTrack(Track* track, Track* destination, int numSamples) noexcept
    {
        int channelsToMix = juce::jmin(destination->audioBuffer.getNumChannels(), track->audioBuffer.getNumChannels());
        for (int c = 0; c < channelsToMix; ++c)
            destination->audioBuffer.addFrom(c, 0, track->audioBuffer, c, 0, numSamples);
    }

};
