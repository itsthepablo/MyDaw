#include "InstrumentPanel.h"
#include "../Theme/CustomTheme.h"

InstrumentPanel::InstrumentPanel() 
{
    addAndMakeVisible(addInstrumentBtn);
    addInstrumentBtn.setButtonText("+ ADD VSTi");

    addInstrumentBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 43, 48));
    addInstrumentBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.6f));

    addInstrumentBtn.onClick = [this] {
        if (activeTrack != nullptr) {
            juce::PopupMenu m;
            m.addItem(1, "Native: Orion (Synth)");
            m.addSeparator();
            m.addItem(2, "External VST3...");

            m.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
                if (result == 1 && onAddNativeInstrument) onAddNativeInstrument(*activeTrack);
                if (result == 2 && onAddInstrument) onAddInstrument(*activeTrack);
            });
        }
    };
}

InstrumentPanel::~InstrumentPanel() 
{
}

void InstrumentPanel::setTrack(Track* t) 
{
    activeTrack = t;
    updateInstrumentView();
}

void InstrumentPanel::updateInstrumentView() 
{
    devices.clear();

    if (activeTrack != nullptr) {
        for (int i = 0; i < (int)activeTrack->plugins.size(); ++i) {
            auto* p = activeTrack->plugins[i];
            if (p && p->getIsInstrument()) {
                // Reutilizamos el componente EffectDevice para mostrar el slot del instrumento.
                // Como EffectDevice requiere una referencia a EffectsPanel, usamos un cast de puntero nulo 
                // para la referencia, asumiendo que el instrumento no usará funciones de ruteo de efectos.
                auto* dev = new EffectDevice(i, p->getLoadedPluginName(), true, p->isBypassed(), p, *static_cast<EffectsPanel*>(nullptr));
                devices.add(dev);
                addAndMakeVisible(dev);
            }
        }
        addInstrumentBtn.setVisible(true);
    }
    else {
        addInstrumentBtn.setVisible(false);
    }

    resized();
    repaint();
}

void InstrumentPanel::paint(juce::Graphics& g) {
    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
        g.fillAll(theme->getSkinColor("INSTRUMENT_BG", juce::Colour(30, 33, 36)));
    } else {
        g.fillAll(juce::Colour(30, 33, 36));
    }

    if (activeTrack == nullptr) {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText("SELECCIONA UNA PISTA MIDI", getLocalBounds(), juce::Justification::centred, true);
    }
    else {
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText("INSTRUMENT CHAIN: " + activeTrack->getName(), 15, 10, 300, 20, juce::Justification::centredLeft);
    }
}

void InstrumentPanel::resized() {
    if (activeTrack != nullptr) {
        auto area = getLocalBounds();
        area.removeFromTop(35);
        area.removeFromBottom(15);
        area.removeFromLeft(15);
        area.removeFromRight(15);

        int padding = 10;
        for (auto* dev : devices) {
            int dWidth = dev->getPreferredWidth();
            dev->setBounds(area.removeFromLeft(dWidth));
            area.removeFromLeft(padding);
        }
        addInstrumentBtn.setBounds(area.removeFromLeft(140));
    }
}