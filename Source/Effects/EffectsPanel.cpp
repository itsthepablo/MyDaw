#include "EffectsPanel.h"
#include "EffectDevice.h"

std::map<void*, bool> EffectsPanel::pluginIsInstrumentMap;

EffectsPanel::EffectsPanel() {
    addAndMakeVisible(closeBtn);
    closeBtn.setButtonText("X");
    closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentWhite);
    closeBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.7f));
    closeBtn.onClick = [this] { if (onClose) onClose(); };

    addAndMakeVisible(addEffectBtn);
    addEffectBtn.setButtonText("+ VST3");
    addEffectBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(70, 75, 80));
    addEffectBtn.onClick = [this] { if (onAddEffect && activeTrack) onAddEffect(*activeTrack); };
    addEffectBtn.setVisible(false);

    addAndMakeVisible(loudnessMeter);
}

void EffectsPanel::setTrack(Track* t) {
    activeTrack = t;
    loudnessMeter.setTrack(t);
    updateSlots();
}

void EffectsPanel::updateSlots() {
    devices.clear();
    addEffectBtn.setVisible(activeTrack != nullptr);

    if (activeTrack) {
        for (int i = 0; i < activeTrack->plugins.size(); ++i) {
            auto* host = activeTrack->plugins[i];
            bool isInst = false;

            if (activeTrack->getType() == TrackType::MIDI) {
                isInst = pluginIsInstrumentMap[(void*)host];
            }

            juce::String name = isInst ? "VSTi Synth" : "VST3 Plugin";
            if (host != nullptr && host->isLoaded()) name = host->getLoadedPluginName();

            bool isBypassed = host != nullptr ? host->isBypassed() : false;

            auto* device = new EffectDevice(i, name, isInst, isBypassed, *this);
            devices.add(device);
            addAndMakeVisible(device);
        }
    }
    resized();
    repaint();
}

void EffectsPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(40, 43, 48));

    g.setColour(juce::Colour(30, 33, 36));
    g.fillRect(0, 0, 150, getHeight());
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawVerticalLine(150, 0.0f, (float)getHeight());

    if (!activeTrack) {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText("SELECCIONA PISTA", 0, 0, getWidth(), getHeight(), juce::Justification::centred);
        return;
    }

    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(16.0f, juce::Font::bold));
    g.drawText("Device Chain", 15, 10, 130, 30, juce::Justification::centredLeft);

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(juce::Font(14.0f));
    g.drawText(activeTrack->getName(), 35, 40, 100, 20, juce::Justification::centredLeft);

    g.setColour(activeTrack->getColor());
    g.fillEllipse(15, 43, 12, 12);
}

void EffectsPanel::resized() {
    closeBtn.setBounds(getWidth() - 35, 5, 30, 30);

    // Ubicamos el medidor numérico dual en el panel izquierdo.
    // Damos espacio suficiente (getHeight() - 80) para el stack vertical de textos.
    loudnessMeter.setBounds(10, 70, 130, getHeight() - 80);

    if (!activeTrack) return;

    int padding = 10;
    int x = 160;
    int y = 20;
    int slotWidth = 140;
    int slotHeight = getHeight() - 40;
    if (slotHeight < 50) slotHeight = 50;

    for (auto* device : devices) {
        device->setBounds(x, y, slotWidth, slotHeight);
        x += slotWidth + padding;
    }

    addEffectBtn.setBounds(x, y + (slotHeight / 2) - 20, 80, 40);
}