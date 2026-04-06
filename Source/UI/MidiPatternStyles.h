#pragma once
#include <JuceHeader.h>

/**
 * @enum MidiStyleType
 * Define la estética visual de la representación de notas MIDI en la Playlist.
 */
enum class MidiStyleType {
    Classic = 0,    // Color de pista de fondo, notas negras
    Modern = 1,     // Fondo oscuro, notas color de pista
    Minimal = 2,    // Fondo transparente, notas color de pista
    Gradient = 3    // Fondo negro, notas con degradado brillante y redondeadas
};

/**
 * @struct MidiStyleRecipe
 * Contiene los parámetros específicos de dibujo para un estilo.
 */
struct MidiStyleRecipe {
    juce::Colour backgroundColor;
    juce::Colour noteColor;
    juce::Colour activeNoteColor;
    juce::Colour noteBorder;
    bool drawBackground = true;
    bool drawHeaderBackground = true;
    bool useGradient = false;
    bool activeUseGradient = false;
    bool useBackgroundGradient = false;
    float noteCornerRadius = 1.0f;
};

/**
 * @class MidiStyleRegistry
 * Clase de utilidad estática para obtener la "receta" de dibujo basada en el estilo y el color base.
 */
class MidiStyleRegistry {
public:
    static MidiStyleRecipe getRecipe(MidiStyleType style, juce::Colour trackColor)
    {
        MidiStyleRecipe recipe;

        switch (style)
        {
        case MidiStyleType::Classic:
            recipe.backgroundColor = trackColor.darker(0.1f);
            recipe.noteColor = juce::Colours::black;
            recipe.activeNoteColor = juce::Colours::black.withAlpha(0.5f); // Se vuelve traslúcido al sonar
            recipe.activeUseGradient = false; 
            recipe.noteBorder = juce::Colours::black.withAlpha(0.2f);
            recipe.drawBackground = true;
            recipe.drawHeaderBackground = false;
            break;

        case MidiStyleType::Modern:
            recipe.backgroundColor = juce::Colour(20, 20, 20);
            recipe.noteColor = trackColor;
            recipe.activeNoteColor = trackColor.brighter(0.8f);
            recipe.noteBorder = trackColor.brighter(0.5f).withAlpha(0.4f);
            recipe.drawBackground = true;
            recipe.drawHeaderBackground = false; 
            break;

        case MidiStyleType::Minimal:
            recipe.backgroundColor = juce::Colours::transparentBlack;
            recipe.noteColor = trackColor;
            recipe.activeNoteColor = trackColor.brighter(0.8f);
            recipe.noteBorder = trackColor.brighter(0.5f).withAlpha(0.3f);
            recipe.drawBackground = false;
            recipe.drawHeaderBackground = false;
            break;

        case MidiStyleType::Gradient:
            recipe.backgroundColor = juce::Colour(15, 15, 15);
            recipe.noteColor = trackColor;
            recipe.activeNoteColor = trackColor.brighter(0.8f);
            recipe.noteBorder = trackColor.brighter(0.6f).withAlpha(0.5f);
            recipe.drawBackground = true;
            recipe.drawHeaderBackground = false;
            recipe.useGradient = true;
            recipe.useBackgroundGradient = true;
            recipe.noteCornerRadius = 2.5f;
            break;
        }

        return recipe;
    }
};
