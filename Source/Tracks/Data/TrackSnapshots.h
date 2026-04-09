#pragma once
#include <JuceHeader.h>
#include <vector>
#include "../../UI/MidiPatternStyles.h"
#include "../../Data/AutomationData.h"

// ============================================================
// DOUBLE BUFFERING (SNAPSHOTS) — Thread-Safe Audio Access
// ============================================================

/**
 * @struct AudioClipSnapshot
 * Captura ligera de un clip de audio para lectura segura en el motor de audio.
 */
struct AudioClipSnapshot {
    float startX     = 0.0f;
    float width      = 0.0f;
    float offsetX    = 0.0f;
    bool  isMuted    = false;
    bool  isLoaded   = true;
    const juce::AudioBuffer<float>* fileBufferPtr = nullptr;
    int   numChannels = 0;
    int   numSamples  = 0;
};

/**
 * @struct MidiNoteSnapshot
 * Captura ligera de una nota MIDI.
 */
struct MidiNoteSnapshot {
    int    pitch     = 0;
    int    x         = 0;
    int    width     = 0;
    double frequency = 0.0;
};

/**
 * @struct MidiClipSnapshot
 * Captura ligera de un conjunto de notas MIDI agrupadas en un clip.
 */
struct MidiClipSnapshot {
    float startX  = 0.0f;
    float width   = 320.0f;
    float offsetX = 0.0f;
    bool  isMuted = false;
    MidiStyleType style = MidiStyleType::Classic;
    std::vector<MidiNoteSnapshot> notes;
};

/**
 * @struct TrackSnapshot
 * Snapshot completo de una pista (audio, midi, automatización) para procesamiento en paralelo.
 */
struct TrackSnapshot {
    std::vector<AudioClipSnapshot> audioClips;
    std::vector<MidiClipSnapshot>  midiClips;
    std::vector<MidiNoteSnapshot>  notes; 
    std::vector<AutomationClipSnapshot> automations;
};
