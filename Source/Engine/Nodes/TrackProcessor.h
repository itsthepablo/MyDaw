#pragma once
#include <JuceHeader.h>
#include "../Core/AudioClock.h"
#include "../../Data/Track.h"
#include "../../UI/Panels/Effects/EffectsPanel.h"
#include "../Routing/RoutingMatrix.h" 
#include "../Routing/PDCManager.h"

/**
 * TrackProcessor — Motor de procesamiento de audio y MIDI de cada pista.
 * Refactorizado a .cpp para mejorar tiempos de compilación.
 */
class TrackProcessor {
public:
    static void process(Track* track,
        const AudioClock& clock,
        int numSamples,
        bool isPlayingNow,
        bool isStoppingNow,
        const juce::MidiBuffer& previewMidi,
        const RoutingMatrix::TopoState* topo) noexcept;
};
