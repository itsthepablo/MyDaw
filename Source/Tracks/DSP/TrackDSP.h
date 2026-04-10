#pragma once
#include <JuceHeader.h>
#include "../../Engine/SimpleLoudness.h"
#include "../../Engine/SimpleBalance.h"
#include "../../Engine/SimpleMidSide.h"
#include <juce_dsp/juce_dsp.h>
#include "../../NativePlugins/ChannelEQ/ChannelEQ_DSP.h"

/**
 * TrackDSP: Encapsula la lógica de procesamiento de audio específica de un track.
 * Incluye compensación de latencia (PDC), ecualización y medidores.
 */
class TrackDSP {
public:
    TrackDSP();
    ~TrackDSP();

    /**
     * Prepara el procesador para una nueva frecuencia de muestreo.
     */
    void prepare(double sampleRate, int samplesPerBlock);

    /**
     * Procesa un bloque de audio del track.
     */
    void process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    /**
     * Gestiona la asignación del buffer de compensación de latencia.
     */
    void allocatePdcBuffer();

    // --- MIEMBROS DE PROCESAMIENTO ---
    SimpleLoudness preLoudness;
    SimpleLoudness postLoudness;
    SimpleBalance  postBalance;
    SimpleMidSide  postMidSide;
    ChannelEQ_DSP  inlineEQ;

    // --- ESTADO ---
    bool isAnalyzersPrepared = false;

    // --- PDC (COMPENSACIÓN DE LATENCIA) ---
    juce::AudioBuffer<float> pdcBuffer; 
    int pdcWritePtr = 0;
    int currentLatency = 0;
    int requiredDelay = 0;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackDSP)
};
