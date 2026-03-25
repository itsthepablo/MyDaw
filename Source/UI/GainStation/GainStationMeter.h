#pragma once
#include <JuceHeader.h>
#include "../../Tracks/Track.h"

class GainStationMeter : public juce::Component, private juce::Timer {
public:
    GainStationMeter() {
        startTimerHz(15);
    }

    void setTrack(Track* t) {
        activeTrack = t;
        repaint();
    }

    void timerCallback() override {
        if (activeTrack && isVisible()) repaint();
    }

    void paint(juce::Graphics& g) override {
        // ELIMINADO: fillAll y drawRect. Ahora el panel padre se encarga del fondo global.

        if (!activeTrack) {
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.setFont(14.0f);
            g.drawText("No Track", getLocalBounds(), juce::Justification::centred);
            return;
        }

        auto formatLufs = [](float lufs) {
            if (lufs <= -69.0f) return juce::String("--.-");
            return juce::String(lufs, 1);
            };

        int halfH = getHeight() / 2;
        auto preArea = getLocalBounds().removeFromTop(halfH).reduced(5);
        auto postArea = getLocalBounds().removeFromBottom(halfH).reduced(5);

        // ELIMINADO: La línea horizontal central. Ahora el panel padre la dibuja completa.

        auto drawSection = [&](juce::Rectangle<int> area, juce::String title, float lufs, juce::Colour color) {
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.setFont(juce::Font("Sans-Serif", 11.0f, juce::Font::bold));
            g.drawText(title, area.removeFromTop(15), juce::Justification::centred);

            // CORRECCIÓN CRÍTICA: La línea de -18 dBFS ya no tacha el número.
            g.setColour(juce::Colours::cyan.withAlpha(0.3f));
            g.drawHorizontalLine(area.getY() + 2, (float)area.getX() + 8.0f, (float)area.getRight() - 8.0f);

            area.removeFromTop(8); // Espacio de respiración tras la línea cyan

            g.setColour(color);
            g.setFont(juce::Font("Sans-Serif", 28.0f, juce::Font::bold));
            g.drawText(formatLufs(lufs), area.removeFromTop(32), juce::Justification::centred);

            g.setColour(color.withAlpha(0.6f));
            g.setFont(juce::Font("Sans-Serif", 11.0f, juce::Font::plain));
            g.drawText("LUFS", area.removeFromTop(15), juce::Justification::centredTop);
            };

        drawSection(preArea, "INPUT PRE-FX", activeTrack->preLoudness.getShortTerm(), juce::Colours::white);
        drawSection(postArea, "OUTPUT POST-FX", activeTrack->postLoudness.getShortTerm(), juce::Colours::orange);
    }

private:
    Track* activeTrack = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainStationMeter)
};