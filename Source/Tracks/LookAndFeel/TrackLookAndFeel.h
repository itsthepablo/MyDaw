#pragma once
#include <JuceHeader.h>
#include "../../Mixer/LookAndFeel/MixerLevelMeterLF.h"

/**
 * TrackLookAndFeel: Controla el diseño visual y look-and-feel de los componentes de track.
 * Permite centralizar el diseño de botones, knobs y paneles de control.
 */
class TrackLookAndFeel : public juce::LookAndFeel_V4 {
public:
    TrackLookAndFeel() {
        // Configuraciones de colores por defecto para tracks
        setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 52));
        setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.8f));
    }

    // Sobrescribir métodos de dibujo aquí para personalizar el diseño
    
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackLookAndFeel)
};
