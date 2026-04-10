#include "UtilityPlugin.h"
#include "UtilityEditor.h"

// ==============================================================================
// UTILITY EDITOR IMPLEMENTATION
// ==============================================================================

void UtilityEditor::paint(juce::Graphics& g) {
    auto area = getLocalBounds();

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(juce::Font("Sans-Serif", 11.0f, juce::Font::bold));

    int halfW = area.getWidth() / 2;
    g.drawText("GAIN", 0, area.getBottom() - 20, halfW, 15, juce::Justification::centred);
    g.drawText("PAN", halfW, area.getBottom() - 20, halfW, 15, juce::Justification::centred);
}

void UtilityEditor::resized() {
    auto area = getLocalBounds().reduced(5);
    area.removeFromTop(5);
    area.removeFromBottom(25);

    int halfW = area.getWidth() / 2;
    gainSlider.setBounds(area.removeFromLeft(halfW).reduced(2));
    panSlider.setBounds(area.reduced(2));
}

// ==============================================================================
// UTILITY PLUGIN IMPLEMENTATION
// ==============================================================================

UtilityPlugin::UtilityPlugin() {
    gain.store(1.0f);
    pan.store(0.0f);
    editor = std::make_unique<UtilityEditor>(gain, pan);
}

void UtilityPlugin::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    if (bypassed) return;

    float p = pan.load();
    float g = gain.load();

    if (buffer.getNumChannels() >= 2) {
        float* left = buffer.getWritePointer(0);
        float* right = buffer.getWritePointer(1);

        float norm = 1.0f / std::sqrt(1.0f + std::abs(p));

        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float inL = left[i];
            float inR = right[i];

            float outL = inL;
            float outR = inR;

            if (p < 0.0f) {
                float panVal = -p;
                outL = inL + (inR * panVal);
                outR = inR * (1.0f - panVal);
            }
            else if (p > 0.0f) {
                outL = inL * (1.0f - p);
                outR = inR + (inL * p);
            }

            left[i] = outL * norm * g;
            right[i] = outR * norm * g;
        }
    }
    else if (buffer.getNumChannels() == 1) {
        buffer.applyGain(0, 0, buffer.getNumSamples(), g);
    }
}

juce::Component* UtilityPlugin::getCustomEditor() {
    return editor.get();
}
