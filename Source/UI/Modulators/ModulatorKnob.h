#pragma once
#include <JuceHeader.h>

/**
 * ModulatorKnob: Un control circular (Knob) estilizado para el sistema de modulación.
 * Sigue los estándares de rules.md: Estética comercial y precisión técnica.
 */
class ModulatorKnob : public juce::Slider {
public:
    ModulatorKnob(const juce::String& labelText) : label(labelText) {
        setSliderStyle(juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        
        addAndMakeVisible(titleLabel);
        titleLabel.setText(label, juce::dontSendNotification);
        titleLabel.setFont(juce::Font(12.0f, juce::Font::bold));
        titleLabel.setJustificationType(juce::Justification::centred);
        titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        
        addAndMakeVisible(valueLabel);
        valueLabel.setFont(juce::Font(10.0f));
        valueLabel.setJustificationType(juce::Justification::centred);
        valueLabel.setColour(juce::Label::textColourId, juce::Colours::cyan.withAlpha(0.8f));
        
        updateValueText(); 
    }

    void paint(juce::Graphics& g) override {
        auto area = getLocalBounds().toFloat();
        auto knobArea = area.reduced(5).withTrimmedTop(15).withTrimmedBottom(15);
        
        float radius = juce::jmin(knobArea.getWidth(), knobArea.getHeight()) / 2.0f;
        float centerX = knobArea.getCentreX();
        float centerY = knobArea.getCentreY();
        float rx = centerX - radius;
        float ry = centerY - radius;
        float rw = radius * 2.0f;
        
        g.setColour(juce::Colour(30, 32, 35));
        g.fillEllipse(rx, ry, rw, rw);
        
        g.setColour(juce::Colour(50, 55, 60));
        g.drawEllipse(rx, ry, rw, rw, 1.5f);

        float angleStart = juce::MathConstants<float>::pi * 1.2f;
        float angleEnd = juce::MathConstants<float>::pi * 2.8f;
        float currentAngle = angleStart + (float)valueToProportionOfLength(getValue()) * (angleEnd - angleStart);
        
        juce::Path arc;
        arc.addCentredArc(centerX, centerY, radius - 3.0f, radius - 3.0f, 0.0f, angleStart, currentAngle, true);
        
        g.setColour(juce::Colour(100, 200, 255));
        g.strokePath(arc, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        
        g.setColour(juce::Colour(100, 200, 255).withAlpha(0.2f));
        g.strokePath(arc, juce::PathStrokeType(6.0f));

        juce::Path p;
        p.addRectangle(-1.5f, -radius, 3.0f, radius * 0.4f);
        g.setColour(juce::Colours::white);
        g.fillPath(p, juce::AffineTransform::rotation(currentAngle).translated(centerX, centerY));
    }

    void resized() override {
        auto area = getLocalBounds();
        titleLabel.setBounds(area.removeFromTop(15));
        valueLabel.setBounds(area.removeFromBottom(15));
    }

    void valueChanged() override {
        updateValueText();
        repaint();
    }

    void updateValueText() {
        if (isRate) {
            // Posiciones alineadas por DURACIÓN (Serum Sync Standard)
            int idx = juce::jlimit(0, 23, (int)std::round(getValue()));
            juce::String names[] = { 
                "8/1", "4/1", "2/1", "1/1D", "1/1", "1/2D", "1/1T", "1/2", 
                "1/4D", "1/2T", "1/4", "1/8D", "1/4T", "1/8", "1/16D", 
                "1/8T", "1/16", "1/32D", "1/16T", "1/32", "1/32T", 
                "1/64", "1/64T", "1/128"
            };
            valueLabel.setText(names[idx], juce::dontSendNotification);
        } else {
            valueLabel.setText(juce::String((int)(getValue() * 100)) + "%", juce::dontSendNotification);
        }
    }

    bool isRate = false;

private:
    juce::String label;
    juce::Label titleLabel;
    juce::Label valueLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorKnob)
};
