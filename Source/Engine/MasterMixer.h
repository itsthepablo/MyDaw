#pragma once
#include <JuceHeader.h>

class MasterMixer {
public:
    static void processTrackVolumeAndRouting(Track* track,
        int numSamples,
        int hardwareOutChannels,
        int parentIndex,
        const juce::OwnedArray<Track>& allTracks, // <--- EL ARREGLO ESTÁ AQUÍ
        const juce::AudioSourceChannelInfo& bufferToFill)
    {
        // 1. Medir Picos para los vúmetros visuales
        if (track->audioBuffer.getNumChannels() > 0) {
            track->currentPeakLevelL = track->audioBuffer.getMagnitude(0, 0, numSamples);
            if (track->audioBuffer.getNumChannels() > 1)
                track->currentPeakLevelR = track->audioBuffer.getMagnitude(1, 0, numSamples);
            else
                track->currentPeakLevelR = track->currentPeakLevelL;
        }

        // 2. Aplicar Volumen y Paneo
        float v = track->getVolume();
        float pan = track->getBalance();

        if (hardwareOutChannels >= 2) {
            track->audioBuffer.applyGain(0, 0, numSamples, v * juce::jmin(1.0f, 1.0f - pan));
            track->audioBuffer.applyGain(1, 0, numSamples, v * juce::jmin(1.0f, 1.0f + pan));
        }

        // 3. Mezclar (Routing a carpeta padre o al Master final)
        if (parentIndex != -1) {
            for (int c = 0; c < hardwareOutChannels; ++c)
                allTracks[parentIndex]->audioBuffer.addFrom(c, 0, track->audioBuffer, c, 0, numSamples);
        }
        else {
            for (int c = 0; c < hardwareOutChannels; ++c)
                bufferToFill.buffer->addFrom(c, bufferToFill.startSample, track->audioBuffer, c, 0, numSamples);
        }
    }
};