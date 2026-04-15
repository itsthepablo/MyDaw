#pragma once
#include <JuceHeader.h>
#include "../../../Data/Track.h"
#include "../../../Engine/Modulation/ModulationTargets.h"

/**
 * ModMappingList: Panel lateral que lista todos los mapeos activos de la pista.
 */
class ModMappingList : public juce::Component, public juce::Timer {
public:
    ModMappingList() {
        startTimerHz(30); // Para actualizar los medidores de actividad
    }

    void setTrack(Track* t) {
        currentTrack = t;
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto area = getLocalBounds();
        
        // Fondo
        g.setColour(juce::Colour(30, 32, 35));
        g.fillRect(area);
        
        // Título
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText("ACTIVE MAPPINGS", 10, 0, getWidth() - 20, 35, juce::Justification::centredLeft);
        
        g.setColour(juce::Colour(60, 65, 70));
        g.drawLine(0, 35, getWidth(), 35, 1.0f);

        if (!currentTrack || currentTrack->modulators.isEmpty()) {
            g.setColour(juce::Colours::grey);
            g.setFont(12.0f);
            g.drawText("No mappings active", area.withTrimmedTop(40), juce::Justification::centred);
            return;
        }

        // Listar mapeos
        int y = 45;
        for (auto* m : currentTrack->modulators) {
            juce::ScopedLock sl(m->targetsLock);
            for (const auto& target : m->targets) {
                drawMappingItem(g, 10, y, getWidth() - 20, 30, *m, target);
                y += 35;
            }
        }
    }

    void timerCallback() override {
        repaint();
    }

private:
    void drawMappingItem(juce::Graphics& g, int x, int y, int w, int h, GridModulator& m, const ModTarget& t) {
        juce::Rectangle<int> area(x, y, w, h);
        
        g.setColour(juce::Colour(45, 48, 52));
        g.fillRoundedRectangle(area.toFloat(), 4.0f);
        
        juce::String targetName = getTargetName(t);
        
        g.setColour(juce::Colours::white);
        g.setFont(12.0f);
        g.drawText(targetName, x + 5, y, w - 40, h, juce::Justification::centredLeft);
        
        // Medidor de actividad mini
        float activity = m.getValueAt(juce::Time::getMillisecondCounterHiRes() * 0.001); // Ficticio para visualización
        auto meterArea = area.removeFromRight(30).reduced(5);
        g.setColour(juce::Colour(10, 15, 20));
        g.fillRect(meterArea);
        
        g.setColour(juce::Colour(100, 200, 255));
        int meterH = (int)(meterArea.getHeight() * (std::abs(activity) / 1.0f));
        g.fillRect(meterArea.getX(), meterArea.getCentreY() - (meterH / 2), meterArea.getWidth(), meterH);
    }

    juce::String getTargetName(const ModTarget& t) {
        if (t.type == ModTarget::PluginParam) {
            if (currentTrack && t.pluginIdx >= 0 && t.pluginIdx < currentTrack->plugins.size()) {
                auto* p = currentTrack->plugins[t.pluginIdx];
                return p->getLoadedPluginName() + " Param #" + juce::String(t.parameterIdx);
            }
        }
        return ModTarget::getName(t.type);
    }

    Track* currentTrack = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMappingList)
};
