#pragma once
#include <JuceHeader.h>

class BpmDragControl : public juce::Component {
public:
    BpmDragControl() {
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(25, 30, 35));
        g.setColour(juce::Colour(60, 65, 70));
        g.drawRect(getLocalBounds(), 1);
        
        g.setColour(juce::Colours::orange);
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText(juce::String(bpm, 1), getLocalBounds().withBottom(getLocalBounds().getBottom() - 10), juce::Justification::centred);
        
        g.setColour(juce::Colours::grey);
        g.setFont(juce::Font(10.0f, juce::Font::plain));
        g.drawText("BPM", getLocalBounds().withTop(getLocalBounds().getBottom() - 14), juce::Justification::centred);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        lastY = e.position.y;
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        float delta = lastY - e.position.y;
        if (std::abs(delta) > 0) {
            float multiplier = e.mods.isShiftDown() ? 0.1f : 1.0f;
            bpm += delta * multiplier;
            bpm = juce::jlimit(40.0f, 240.0f, bpm);
            lastY = e.position.y;
            repaint();
            if (onBpmChanged) onBpmChanged(bpm);
        }
    }

    float getBpm() const { return bpm; }

    std::function<void(float)> onBpmChanged;

private:
    float bpm = 120.0f;
    float lastY = 0;
};