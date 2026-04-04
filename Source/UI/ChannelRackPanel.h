#pragma once
#include <JuceHeader.h>
#include <vector>
#include "../Theme/CustomTheme.h"
#include "Knobs/FLKnobLookAndFeel.h"

// ==============================================================================
// 1. ESTRUCTURA DE DATOS GLOBAL (Desconectada de las Pistas)
// ==============================================================================
struct RackChannel {
    juce::String name;
    float volume = 0.8f;
    float pan = 0.0f;
    bool isMuted = false;
    bool steps[16] = { false };
    juce::Colour color = juce::Colour(90, 95, 100);
};

// ==============================================================================
// 2. BOTÓN DE STEP (Bloque sólido FL Studio)
// ==============================================================================
class StepButton : public juce::Component {
public:
    StepButton(int index, bool& stateRef) : stepIndex(index), isActive(stateRef) {
        // Colores alternos cada 4 pasos
        if ((stepIndex / 4) % 2 == 0) {
            offColor = juce::Colour(75, 80, 85);
        }
        else {
            offColor = juce::Colour(100, 65, 65);
        }
        onColor = juce::Colour(200, 210, 220);
    }

    void paint(juce::Graphics& g) override {
        juce::Rectangle<int> bounds = getLocalBounds().reduced(1, 2);

        g.setColour(isActive ? onColor : offColor);
        g.fillRoundedRectangle(bounds.toFloat(), 2.0f);

        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawRoundedRectangle(bounds.toFloat(), 2.0f, 1.0f);

        g.setColour(juce::Colours::white.withAlpha(isActive ? 0.4f : 0.05f));
        g.drawHorizontalLine(bounds.getY() + 1, (float)bounds.getX() + 2, (float)bounds.getRight() - 2);
    }

    void mouseDown(const juce::MouseEvent&) override {
        isActive = !isActive;
        repaint();
    }

private:
    int stepIndex;
    bool& isActive;
    juce::Colour offColor, onColor;
};

// ==============================================================================
// 3. FILA INDIVIDUAL DEL RACK (Mute -> Pan -> Vol -> Nombre -> Steps)
// ==============================================================================
class ChannelRackRow : public juce::Component {
public:
    ChannelRackRow(RackChannel& ch) : channel(ch) {

        addAndMakeVisible(muteBtn);
        muteBtn.setClickingTogglesState(true);
        muteBtn.setToggleState(!channel.isMuted, juce::dontSendNotification);
        muteBtn.onClick = [this] { channel.isMuted = !muteBtn.getToggleState(); };

        addAndMakeVisible(panSlider);
        panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panSlider.setLookAndFeel(&knobLookAndFeel); // Usa el estilo de la carpeta Knobs
        panSlider.setRange(-1.0, 1.0, 0.01);
        panSlider.setValue(channel.pan);
        panSlider.onValueChange = [this] { channel.pan = (float)panSlider.getValue(); };

        addAndMakeVisible(volSlider);
        volSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        volSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        volSlider.setLookAndFeel(&knobLookAndFeel); // Usa el estilo de la carpeta Knobs
        volSlider.setRange(0.0, 1.0, 0.01);
        volSlider.setValue(channel.volume);
        volSlider.onValueChange = [this] { channel.volume = (float)volSlider.getValue(); };

        for (int i = 0; i < 16; ++i) {
            auto* step = steps.add(new StepButton(i, channel.steps[i]));
            addAndMakeVisible(step);
        }
    }

    void paint(juce::Graphics& g) override {
        if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
            g.fillAll(theme->getSkinColor("CHANNEL_RACK_BG", juce::Colour(30, 32, 35)).brighter(0.1f));
        } else {
            g.fillAll(juce::Colour(45, 48, 52));
        }
        
        g.setColour(juce::Colour(30, 32, 35));
        g.drawHorizontalLine(getHeight() - 1, 0, (float)getWidth());

        g.setColour(muteBtn.getToggleState() ? juce::Colours::limegreen : juce::Colour(60, 60, 60));
        g.fillEllipse(8, (float)getHeight() / 2 - 4, 8, 8);

        juce::Rectangle<int> nameBounds(85, 4, 100, getHeight() - 8);
        g.setColour(channel.color);
        g.fillRoundedRectangle(nameBounds.toFloat(), 3.0f);

        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawHorizontalLine(nameBounds.getY() + 1, (float)nameBounds.getX() + 2, (float)nameBounds.getRight() - 2);

        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(13.0f, juce::Font::bold));
        g.drawText(channel.name, nameBounds.reduced(8, 0), juce::Justification::centredLeft, true);
    }

    void lookAndFeelChanged() override {
        repaint();
    }

    void resized() override {
        auto area = getLocalBounds();

        muteBtn.setBounds(area.removeFromLeft(20));
        area.removeFromLeft(5);
        panSlider.setBounds(area.removeFromLeft(24).reduced(0, 4));
        area.removeFromLeft(4);
        volSlider.setBounds(area.removeFromLeft(24).reduced(0, 4));

        area.removeFromLeft(110);

        int numSteps = steps.size();
        float stepW = (float)area.getWidth() / numSteps;
        float currentX = (float)area.getX();

        for (auto* step : steps) {
            step->setBounds((int)currentX, area.getY(), (int)stepW, area.getHeight());
            currentX += stepW;
        }
    }

private:
    RackChannel& channel;
    juce::ToggleButton muteBtn;
    juce::Slider panSlider, volSlider;
    juce::OwnedArray<StepButton> steps;

    FLKnobLookAndFeel knobLookAndFeel; // Instancia de tu clase compartida

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackRow)
};

// ==============================================================================
// 4. COMPONENTE PRINCIPAL DEL RACK
// ==============================================================================
class ChannelRackPanel : public juce::Component {
public:
    ChannelRackPanel() {
        // --- INICIALIZACIÓN DE CANALES GLOBALES ---
        globalChannels.push_back({ "Kick", 0.8f, 0.0f, false, {0}, juce::Colour(90, 100, 110) });
        globalChannels.push_back({ "Clap", 0.8f, 0.0f, false, {0}, juce::Colour(110, 90, 100) });
        globalChannels.push_back({ "HiHat", 0.7f, 0.0f, false, {0}, juce::Colour(100, 110, 90) });
        globalChannels.push_back({ "Snare", 0.8f, 0.0f, false, {0}, juce::Colour(95, 95, 105) });

        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&contentComponent, false);
        viewport.setScrollBarsShown(true, false, true, false);

        rebuildRows();
    }

    void rebuildRows() {
        contentComponent.removeAllChildren();
        rows.clear();

        for (auto& ch : globalChannels) {
            auto* newRow = rows.add(new ChannelRackRow(ch));
            contentComponent.addAndMakeVisible(newRow);
        }
        resized();
    }

    void paint(juce::Graphics& g) override {
        if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
            g.fillAll(theme->getSkinColor("CHANNEL_RACK_BG", juce::Colour(30, 32, 35)));
        } else {
            g.fillAll(juce::Colour(30, 32, 35));
        }
    }

    void lookAndFeelChanged() override {
        repaint();
    }

    void resized() override {
        viewport.setBounds(getLocalBounds());

        int rowHeight = 36;
        int totalHeight = (int)globalChannels.size() * rowHeight;

        int contentW = viewport.getWidth() - (viewport.isVerticalScrollBarShown() ? viewport.getScrollBarThickness() : 0);
        contentComponent.setBounds(0, 0, contentW, totalHeight);

        for (int i = 0; i < rows.size(); ++i) {
            rows[i]->setBounds(0, i * rowHeight, contentComponent.getWidth(), rowHeight);
        }
    }

private:
    std::vector<RackChannel> globalChannels;

    juce::Viewport viewport;
    juce::Component contentComponent;
    juce::OwnedArray<ChannelRackRow> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackPanel)
};