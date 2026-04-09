#pragma once
#include <JuceHeader.h>
#include "../MidiPattern.h"
#include "../LookAndFeel/MidiPatternLF.h"

/**
 * MidiPatternRenderer: Se encarga de dibujar el MidiPattern en la pantalla.
 * Utiliza el MidiPatternLookAndFeel para la estética.
 */
class MidiPatternRenderer {
public:
    static void drawMidiPattern(juce::Graphics& g, 
                               const MidiPattern& pattern, 
                               juce::Rectangle<int> area, 
                               juce::Colour trackColor,
                               const juce::String& name,
                               bool isInlineEditingActive,
                               double hZoom,
                               int hS,
                               double playheadAbsPos,
                               MidiPatternLookAndFeel* lf = nullptr);

    static int hitTestInlineNote(const MidiPattern& pattern,
                                 juce::Rectangle<int> clipRect,
                                 int mouseX, int mouseY,
                                 double hZoom, int hS);

    static void drawMidiSummary(juce::Graphics& g, 
                               const MidiPattern& pattern, 
                               juce::Rectangle<int> area, 
                               float hZoom, 
                               float hS, 
                               int minP, 
                               int maxP);
};
