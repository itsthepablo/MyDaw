#pragma once
#include <JuceHeader.h>
#include "../../../UI/MidiPatternStyles.h"

/**
 * MidiPatternLookAndFeel: Define la estética de los patrones MIDI.
 * Centraliza colores, estilos de notas y fondos basándose en MidiStyleType.
 */
class MidiPatternLookAndFeel : public juce::LookAndFeel_V4 {
public:
    MidiPatternLookAndFeel() {
        setColour(selectedOutlineId, juce::Colours::yellow);
    }

    virtual void drawMidiPatternBackground(juce::Graphics& g, 
                                          juce::Rectangle<float> area, 
                                          juce::Colour trackColor, 
                                          MidiStyleType style,
                                          bool isSelected) 
    {
        auto recipe = MidiStyleRegistry::getRecipe(style, trackColor);
        
        if (recipe.drawBackground) {
            if (recipe.useBackgroundGradient) {
                juce::ColourGradient cg(recipe.backgroundColor, area.getX(), area.getY(),
                                       recipe.backgroundColor.darker(0.3f), area.getX(), area.getBottom(), false);
                g.setGradientFill(cg);
            } else {
                g.setColour(recipe.backgroundColor);
            }
            g.fillRoundedRectangle(area, 5.0f);
        }

        if (isSelected) {
            g.setColour(findColour(selectedOutlineId));
            g.drawRoundedRectangle(area, 5.0f, 1.5f);
        }
    }

    virtual void drawMidiNote(juce::Graphics& g, 
                              juce::Rectangle<float> noteRect, 
                              juce::Colour trackColor, 
                              MidiStyleType style,
                              bool isPlaying) 
    {
        auto recipe = MidiStyleRegistry::getRecipe(style, trackColor);
        juce::Colour c = isPlaying ? recipe.activeNoteColor : recipe.noteColor;
        
        if (recipe.useGradient || (isPlaying && recipe.activeUseGradient)) {
            juce::ColourGradient cg(c.brighter(0.3f), noteRect.getX(), noteRect.getY(),
                                   c.darker(0.2f), noteRect.getX(), noteRect.getBottom(), false);
            g.setGradientFill(cg);
        } else {
            g.setColour(c);
        }
        
        g.fillRoundedRectangle(noteRect, recipe.noteCornerRadius);
        
        if (recipe.noteBorder.getAlpha() > 0.0f) {
            g.setColour(recipe.noteBorder);
            g.drawRoundedRectangle(noteRect, recipe.noteCornerRadius, 0.5f);
        }
    }

    enum ColourIDs {
        selectedOutlineId = 0x2002001
    };
};
