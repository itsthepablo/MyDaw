#include "EffectsPanel.h"
#include "EffectDevice.h"

std::map<void*, bool> EffectsPanel::pluginIsInstrumentMap;

EffectsPanel::EffectsPanel() {

    // --- BOTÓN BYPASS GLOBAL ---
    addAndMakeVisible(bypassAllBtn);
    bypassAllBtn.setButtonText("BYPASS");
    bypassAllBtn.setClickingTogglesState(true);
    bypassAllBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 53));
    bypassAllBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    bypassAllBtn.setTooltip("Apagar / Encender toda la cadena de efectos (A/B Test)");
    bypassAllBtn.onClick = [this] {
        if (activeTrack) {
            bool isBypassed = bypassAllBtn.getToggleState();

            // Recorremos todos los VSTs de la pista y forzamos su estado
            for (auto* p : activeTrack->plugins) {
                if (p) p->setBypassed(isBypassed);
            }
            // Refrescamos visualmente todas las cajas para que coincidan
            updateSlots();
        }
        };

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
    bypassAllBtn.setVisible(activeTrack != nullptr);

    bool allBypassed = true;
    bool hasPlugins = false;

    if (activeTrack) {
        for (int i = 0; i < activeTrack->plugins.size(); ++i) {
            auto* host = activeTrack->plugins[i];

            if (host) {
                hasPlugins = true;
                // Si encontramos al menos un plugin encendido, la cadena no está 100% en bypass
                if (!host->isBypassed()) allBypassed = false;
            }

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

    // Sincronizamos el estado del botón Bypass Global al abrir la pista
    if (hasPlugins && allBypassed) bypassAllBtn.setToggleState(true, juce::dontSendNotification);
    else bypassAllBtn.setToggleState(false, juce::dontSendNotification);

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

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    // Reducimos el ancho del texto para dejarle espacio al botón de Bypass
    g.drawText("EFFECTS", 15, 10, 60, 30, juce::Justification::centredLeft);

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(juce::Font(14.0f));
    g.drawText(activeTrack->getName(), 35, 40, 100, 20, juce::Justification::centredLeft);

    g.setColour(activeTrack->getColor());
    g.fillEllipse(15, 43, 12, 12);
}

void EffectsPanel::resized() {
    int leftPadding = 10;
    int currentY = 70;
    int leftWidth = 130;
    int bottomY = getHeight() - leftPadding;

    // Colocamos el botón de Bypass Global en el panel oscuro, al lado de la palabra "EFFECTS"
    bypassAllBtn.setBounds(85, 15, 55, 20);

    loudnessMeter.setBounds(leftPadding, currentY, leftWidth, bottomY - currentY);

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