#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include <functional>
#include <map> 
#include "EffectDevice.h"

// ==============================================================================
// MEDIDOR LUFS NUMÉRICO DUAL
// ==============================================================================
class LoudnessDualMeter : public juce::Component, private juce::Timer {
public:
    LoudnessDualMeter() {
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
        g.fillAll(juce::Colour(25, 28, 31)); 
        g.setColour(juce::Colour(50, 53, 56));
        g.drawRect(getLocalBounds());

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

        g.setColour(juce::Colour(50, 53, 56));
        g.drawHorizontalLine(halfH, 10, getWidth() - 10);

        auto drawSection = [&](juce::Rectangle<int> area, juce::String title, float lufs, juce::Colour color) {
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.setFont(juce::Font("Sans-Serif", 11.0f, juce::Font::bold));
            g.drawText(title, area.removeFromTop(15), juce::Justification::centred);

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
};

// ==============================================================================
// EFFECTS PANEL (Contenedor Principal)
// ==============================================================================
class EffectsPanel : public juce::Component {
public:
    EffectsPanel();

    void setTrack(Track* t);
    void updateSlots();

    void paint(juce::Graphics& g) override;
    void resized() override;

    Track* getActiveTrack() const { return activeTrack; }

    static std::map<void*, bool> pluginIsInstrumentMap;

    std::function<void(Track&)> onAddVST3;           // <-- Para cargar VST3 normal
    std::function<void(Track&)> onAddNativeUtility;  // <-- NUEVO: Carga el Utility nativo
    std::function<void(Track&, int)> onOpenEffect;
    std::function<void(Track&, int, bool)> onBypassChanged;
    std::function<void(Track&, int, int)> onReorderEffects;
    std::function<void(Track&, int)> onDeleteEffect;

private:
    Track* activeTrack = nullptr;
    
    juce::TextButton bypassAllBtn;
    juce::TextButton addEffectBtn;
    juce::OwnedArray<EffectDevice> devices; 
    LoudnessDualMeter loudnessMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsPanel)
};