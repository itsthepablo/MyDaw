#pragma once
#include <JuceHeader.h>
#include "../../Data/GlobalData.h"
#include "LookAndFeel/MidiPatternStyles.h"
#include "../../Data/AutomationData.h"
#include <vector>

/**
 * MidiPattern: "Caja Negra" que gestiona los datos de un patrón MIDI.
 * Encapsula la lista de notas y la lógica de coordenadas relativas.
 */
class MidiPattern {
public:
    MidiPattern();
    ~MidiPattern() = default;

    // --- COPIA PARA SNAPSHOTS ---
    MidiPattern(const MidiPattern& other);
    MidiPattern& operator=(const MidiPattern& other);

    // --- MIEMBROS DE DATOS (Encapsulados) ---
    juce::String getName() const { return name; }
    void setName(const juce::String& n) { name = n; }

    float getStartX() const { return startX; }
    void setStartX(float x) { startX = x; }

    float getWidth() const { return width; }
    void setWidth(float w) { width = w; }

    float getOffsetX() const { return offsetX; }
    void setOffsetX(float o) { offsetX = o; }

    bool getIsMuted() const { return isMuted; }
    void setIsMuted(bool m) { isMuted = m; }

    bool getIsSelected() const { return isSelected; }
    void setIsSelected(bool s) { isSelected = s; }

    MidiStyleType getStyle() const { return style; }
    void setStyle(MidiStyleType s) { style = s; }

    juce::Colour getColor() const { return color; }
    void setColor(juce::Colour c) { color = c; }

    // --- GESTIÓN DE NOTAS ---
    std::vector<Note>& getNotes() { return notes; }
    const std::vector<Note>& getNotes() const { return notes; }
    
    void migrateToRelative();

    // --- AUTOMATIZACIÓN INTERNA ---
    AutoLane autoVol;
    AutoLane autoPan;
    AutoLane autoPitch;
    AutoLane autoFilter;

private:
    juce::String name;
    float startX = 0.0f;
    float width = 320.0f;
    float offsetX = 0.0f;
    bool isMuted = false;
    bool isSelected = false;
    MidiStyleType style = MidiStyleType::Modern;
    juce::Colour color;
    
    std::vector<Note> notes;

    JUCE_LEAK_DETECTOR(MidiPattern)
};
