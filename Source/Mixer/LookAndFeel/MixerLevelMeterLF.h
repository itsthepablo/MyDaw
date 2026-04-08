#pragma once
#include <JuceHeader.h>

/**
 * Interfaz para permitir que el LevelMeter se dibuje de forma personalizada
 * según el LookAndFeel asignado.
 */
class LevelMeterLookAndFeel {
public:
    virtual ~LevelMeterLookAndFeel() = default;
    virtual void drawLevelMeter(juce::Graphics& g, 
                                juce::Rectangle<float> bounds, 
                                float levelL, float levelR, 
                                float peakL, float peakR, 
                                bool isClipping, bool isHorizontal) = 0;
};

/**
 * Implementación específica para el Mixer con estética Naranja/Ámbar Premium.
 */
class MixerLevelMeterDesigner {
public:
    static void draw(juce::Graphics& g, 
                     juce::Rectangle<float> bounds, 
                     float levelL, float levelR, 
                     float peakL, float peakR, 
                     bool isClipping, bool isHorizontal)
    {
        const float floorDB = -80.0f;
        const auto bgColour = juce::Colour(10, 12, 14);
        const auto clipColour = isClipping ? juce::Colours::orangered : juce::Colour(35, 37, 40);

        if (!isHorizontal) 
        {
            // --- DISEÑO VERTICAL ---
            auto clipArea = bounds.removeFromTop(4.0f);
            g.setColour(clipColour);
            g.fillRect(clipArea.reduced(0.5f, 0.5f));

            bounds.removeFromTop(2.0f);
            g.setColour(bgColour);
            g.fillRoundedRectangle(bounds, 2.0f);

            auto drawBar = [&](float currentVol, float maxPeak, juce::Rectangle<float> area) {
                float db = juce::Decibels::gainToDecibels(currentVol, floorDB);
                float prop = juce::jlimit(0.0f, 1.0f, juce::jmap(db, floorDB, 0.0f, 0.0f, 1.0f));

                float peakDb = juce::Decibels::gainToDecibels(maxPeak, floorDB);
                float peakProp = juce::jlimit(0.0f, 1.0f, juce::jmap(peakDb, floorDB, 0.0f, 0.0f, 1.0f));

                // Gradiente Naranja Premium (Amber -> Bright Orange)
                juce::ColourGradient gradient(juce::Colour(255, 140, 0), area.getBottomLeft(), // Amber
                                             juce::Colour(255, 200, 50), area.getTopLeft(), false); // Golden Orange
                
                gradient.addColour(0.7, juce::Colour(255, 165, 0)); // Orange
                gradient.addColour(0.3, juce::Colour(200, 100, 0)); // Dark Amber

                g.setGradientFill(gradient);
                float barHeight = prop * area.getHeight();
                if (barHeight > 0.5f) g.fillRect(area.withTop(area.getBottom() - barHeight));

                // Indicador de pico (blanco sutil)
                if (peakProp > 0.05f) {
                    float py = area.getBottom() - (peakProp * area.getHeight());
                    g.setColour(juce::Colours::white.withAlpha(0.6f));
                    g.fillRect(area.getX(), py, area.getWidth(), 1.0f);
                }
            };

            float halfW = bounds.getWidth() / 2.0f;
            drawBar(levelL, peakL, bounds.removeFromLeft(halfW).reduced(1.0f, 0.0f));
            drawBar(levelR, peakR, bounds.reduced(1.0f, 0.0f));
        }
        else 
        {
            // --- DISEÑO HORIZONTAL ---
            auto clipArea = bounds.removeFromRight(4.0f);
            g.setColour(clipColour);
            g.fillRect(clipArea.reduced(0.5f, 0.5f));

            bounds.removeFromRight(2.0f);
            g.setColour(bgColour);
            g.fillRoundedRectangle(bounds, 2.0f);

            auto drawHorizontalBar = [&](float currentVol, float maxPeak, juce::Rectangle<float> area) {
                float db = juce::Decibels::gainToDecibels(currentVol, floorDB);
                float prop = juce::jlimit(0.0f, 1.0f, juce::jmap(db, floorDB, 0.0f, 0.0f, 1.0f));

                float peakDb = juce::Decibels::gainToDecibels(maxPeak, floorDB);
                float peakProp = juce::jlimit(0.0f, 1.0f, juce::jmap(peakDb, floorDB, 0.0f, 0.0f, 1.0f));

                juce::ColourGradient gradient(juce::Colour(200, 100, 0), area.getTopLeft(),
                                             juce::Colour(255, 200, 50), area.getTopRight(), false);
                g.setGradientFill(gradient);

                float barWidth = prop * area.getWidth();
                if (barWidth > 0.5f) g.fillRect(area.withWidth(barWidth));

                if (peakProp > 0.05f) {
                    float px = area.getX() + (peakProp * area.getWidth());
                    g.setColour(juce::Colours::white.withAlpha(0.6f));
                    g.fillRect(px, area.getY(), 1.0f, area.getHeight());
                }
            };

            float halfH = bounds.getHeight() / 2.0f;
            drawHorizontalBar(levelL, peakL, bounds.removeFromTop(halfH).reduced(0.0f, 1.0f));
            drawHorizontalBar(levelR, peakR, bounds.reduced(0.0f, 1.0f));
        }
    }
};
