#include "TrackDSP.h"

TrackDSP::TrackDSP()
{
}

TrackDSP::~TrackDSP()
{
}

void TrackDSP::prepare(double sampleRate, int samplesPerBlock)
{
    inlineEQ.prepare(sampleRate, samplesPerBlock, 2);
    // Agregaremos medidores y medidor de loudness en una futura actualización del motor
}

void TrackDSP::process(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    // El motor de audio actualmente procesa cada track individualmente.
    // Esta clase servirá para centralizar el procesado que actualmente está disperso.
    ignoreUnused(buffer, midiMessages);
}

void TrackDSP::allocatePdcBuffer()
{
    // PDC Ring Buffer - Reserva 4MB para compensar hasta ~500,000 samples (aprox 10 segundos)
    if (pdcBuffer.getNumSamples() >= 524288) return; 
    
    pdcBuffer.setSize(2, 524288, false, true, false);
    pdcBuffer.clear();
    pdcWritePtr = 0;
}
