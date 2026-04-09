#pragma once
#include <JuceHeader.h>
#include "../../GlobalData.h"
#include "../../UI/MidiPatternStyles.h"
#include "../../Data/AutomationData.h"
#include <vector>

/**
 * @struct MidiClipData
 * Almacena la información de un clip MIDI en la playlist (posiciones, notas, automatización interna).
 */
struct MidiClipData {
    juce::String name;
    float startX = 0.0f;
    float width = 320.0f;
    float offsetX = 0.0f;
    bool isMuted = false;
    bool isSelected = false;
    std::vector<Note> notes;
    juce::Colour color;
    MidiStyleType style = MidiStyleType::Classic;

    AutoLane autoVol;
    AutoLane autoPan;
    AutoLane autoPitch;
    AutoLane autoFilter;
};
