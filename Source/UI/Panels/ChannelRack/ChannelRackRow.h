#pragma once
#include <JuceHeader.h>
#include "RackChannel.h"
#include "../../Knobs/FLKnobLookAndFeel.h"
#include "../../../Theme/CustomTheme.h"

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
        panSlider.setLookAndFeel(&knobLookAndFeel); 
        panSlider.setRange(-1.0, 1.0, 0.01);
        panSlider.setValue(channel.pan);
        panSlider.onValueChange = [this] { channel.pan = (float)panSlider.getValue(); };

        addAndMakeVisible(volSlider);
        volSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        volSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        volSlider.setLookAndFeel(&knobLookAndFeel); 
        volSlider.setRange(0.0, 1.0, 0.01);
        volSlider.setValue(channel.volume);
        volSlider.onValueChange = [this] { channel.volume = (float)volSlider.getValue(); };

        for (int i = 0; i < 16; ++i) {
            auto* step = steps.add(new StepButton(i, channel.steps[i]));
            addAndMakeVisible(step);
        }
    }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override { repaint(); }

private:
    RackChannel& channel;
    juce::ToggleButton muteBtn;
    juce::Slider panSlider, volSlider;
    juce::OwnedArray<StepButton> steps;

    FLKnobLookAndFeel knobLookAndFeel; 

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackRow)
};
