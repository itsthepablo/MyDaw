#include "MidiPattern.h"

MidiPattern::MidiPattern()
{
}

MidiPattern::MidiPattern(const MidiPattern& other)
{
    *this = other;
}

MidiPattern& MidiPattern::operator=(const MidiPattern& other)
{
    if (this != &other) {
        name = other.name;
        startX = other.startX;
        width = other.width;
        offsetX = other.offsetX;
        isMuted = other.isMuted;
        isSelected = other.isSelected;
        style = other.style;
        color = other.color;
        notes = other.notes;
        autoVol = other.autoVol;
        autoPan = other.autoPan;
        autoPitch = other.autoPitch;
        autoFilter = other.autoFilter;
    }
    return *this;
}

void MidiPattern::migrateToRelative()
{
    for (auto& note : notes) {
        if (note.x >= (int)startX && note.x < (int)(startX + width + 1.0f)) {
            note.x -= (int)startX;
        }
    }
}
