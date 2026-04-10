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

/**
 * TrackLevelMeterLF: LookAndFeel del medidor de peak del track.
 * Sincronizado dinamicamente con el color del track elegido por el usuario.
 */
class TrackLevelMeterLF : public juce::LookAndFeel_V4,
                          public LevelMeterLookAndFeel {
public:
    TrackLevelMeterLF() {}

    void setTrackColour(juce::Colour c) { trackColour = c; }

    void drawLevelMeter(juce::Graphics& g,
                        juce::Rectangle<float> bounds,
                        float levelL, float levelR,
                        float peakL, float peakR,
                        bool isClipping, bool isHorizontal) override
    {
        const float floorDB = -80.0f;
        const auto bgColour = juce::Colour(10, 12, 14);
        
        // Indicador de clip (rojo si clipea, gris oscuro si no)
        auto clipArea = bounds.removeFromTop(4.0f);
        g.setColour(isClipping ? juce::Colours::red : juce::Colour(35, 37, 40));
        g.fillRect(clipArea.reduced(0.5f, 0.5f));

        bounds.removeFromTop(2.0f);
        g.setColour(bgColour);
        g.fillRoundedRectangle(bounds, 2.0f);

        auto drawBar = [&](float currentVol, float maxPeak, juce::Rectangle<float> area) {
            float db = juce::Decibels::gainToDecibels(currentVol, floorDB);
            float prop = juce::jlimit(0.0f, 1.0f, juce::jmap(db, floorDB, 0.0f, 0.0f, 1.0f));

            float peakDb = juce::Decibels::gainToDecibels(maxPeak, floorDB);
            float peakProp = juce::jlimit(0.0f, 1.0f, juce::jmap(peakDb, floorDB, 0.0f, 0.0f, 1.0f));

            // Gradiente basado integramente en el color del track
            juce::ColourGradient gradient(trackColour.darker(0.6f), area.getBottomLeft(),
                                          trackColour.brighter(0.4f), area.getTopLeft(), false);
            gradient.addColour(0.5, trackColour);

            g.setGradientFill(gradient);
            float barHeight = prop * area.getHeight();
            if (barHeight > 0.5f) g.fillRect(area.withTop(area.getBottom() - barHeight));

            // Indicador de pico eliminado (Diseño Moderno)
        };

        float halfW = bounds.getWidth() / 2.0f;
        drawBar(levelL, peakL, bounds.removeFromLeft(halfW).reduced(1.0f, 0.0f));
        drawBar(levelR, peakR, bounds.reduced(1.0f, 0.0f));
        (void)isHorizontal;
    }

private:
    juce::Colour trackColour = juce::Colour(80, 200, 120);
};
