#include "EffectsPanel.h"
#include "EffectDevice.h"

std::map<void*, bool> EffectsPanel::pluginIsInstrumentMap;

EffectsPanel::EffectsPanel() {

    addAndMakeVisible(bypassAllBtn);
    bypassAllBtn.setButtonText("BYPASS");
    bypassAllBtn.setClickingTogglesState(true);
    bypassAllBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 53));
    bypassAllBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    bypassAllBtn.setTooltip("Apagar / Encender toda la cadena de efectos (A/B Test)");
    bypassAllBtn.onClick = [this] {
        if (activeTrack) {
            bool isBypassed = bypassAllBtn.getToggleState();
            for (auto* p : activeTrack->plugins) {
                if (p) p->setBypassed(isBypassed);
            }
            updateSlots();
        }
        };

    addAndMakeVisible(addEffectBtn);
    addEffectBtn.setButtonText("+ PLUGIN");
    addEffectBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(70, 75, 80));

    // --- MENÚ DESPLEGABLE PARA ELEGIR VST O NATIVO ---
    addEffectBtn.onClick = [this] {
        if (activeTrack) {
            juce::PopupMenu m;
            m.addItem(1, "Native: Utility (Gain/Pan)");
            m.addSeparator();
            m.addItem(2, "External VST3...");

            m.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
                if (result == 1 && onAddNativeUtility) onAddNativeUtility(*activeTrack);
                if (result == 2 && onAddVST3) onAddVST3(*activeTrack);
                });
        }
        };

    addEffectBtn.setVisible(false);

    // --- BARRA NEGRA (SOLO VISIBLE CUANDO ESTÁ OCULTO) ---
    addAndMakeVisible(toggleGainStationBtn);
    toggleGainStationBtn.setButtonText(">");
    toggleGainStationBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(15, 18, 20)); // Barra oscura
    toggleGainStationBtn.setTooltip("Mostrar Gain Station");
    toggleGainStationBtn.onClick = [this] {
        isGainStationExpanded = true;
        resized();
        repaint();
        };

    // --- BOTÓN HIDE (SOLO VISIBLE CUANDO ESTÁ EXPANDIDO) ---
    addAndMakeVisible(hideGainStationBtn);
    hideGainStationBtn.setButtonText("HIDE");
    // Diseño minimalista e integrado
    hideGainStationBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    hideGainStationBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.4f));
    hideGainStationBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    hideGainStationBtn.onClick = [this] {
        isGainStationExpanded = false;
        resized();
        repaint();
        };

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
            auto* effectRef = activeTrack->plugins[i];

            if (effectRef) {
                hasPlugins = true;
                if (!effectRef->isBypassed()) allBypassed = false;
            }

            bool isInst = false;
            if (activeTrack->getType() == TrackType::MIDI && !effectRef->isNative()) {
                isInst = pluginIsInstrumentMap[(void*)effectRef];
            }

            juce::String name = isInst ? "VSTi Synth" : "Plugin";
            if (effectRef != nullptr && effectRef->isLoaded()) name = effectRef->getLoadedPluginName();

            bool isBypassed = effectRef != nullptr ? effectRef->isBypassed() : false;

            // Pasamos effectRef al EffectDevice
            auto* device = new EffectDevice(i, name, isInst, isBypassed, effectRef, *this);
            devices.add(device);
            addAndMakeVisible(device);
        }
    }

    if (hasPlugins && allBypassed) bypassAllBtn.setToggleState(true, juce::dontSendNotification);
    else bypassAllBtn.setToggleState(false, juce::dontSendNotification);

    resized();
    repaint();
}

void EffectsPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(40, 43, 48)); // Fondo principal donde van los plugins

    int trackInfoWidth = 110;
    int gainStationWidth = isGainStationExpanded ? 180 : 0;
    int totalLeftWidth = trackInfoWidth + gainStationWidth; // El panel unificado

    // --- FONDO UNIFICADO DEL PANEL IZQUIERDO ---
    // Esto hace que la Gain Station parezca una extensión nativa de los controles de la pista
    g.setColour(juce::Colour(30, 33, 36));
    g.fillRect(0, 0, totalLeftWidth, getHeight());
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawVerticalLine(totalLeftWidth, 0.0f, (float)getHeight());

    if (!activeTrack) {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText("SELECCIONA PISTA", 0, 0, getWidth(), getHeight(), juce::Justification::centred);
        return;
    }

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("EFFECTS", 15, 10, 60, 30, juce::Justification::centredLeft);

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(juce::Font(14.0f));
    g.drawText(activeTrack->getName(), 35, 40, trackInfoWidth - 40, 20, juce::Justification::centredLeft);

    g.setColour(activeTrack->getColor());
    g.fillEllipse(15, 43, 12, 12);
}

void EffectsPanel::resized() {
    int trackInfoWidth = 110;
    int gainStationWidth = 180;
    int toggleBarWidth = 20;
    int padding = 15;

    bypassAllBtn.setBounds(15, 70, 80, 20);

    if (!activeTrack) {
        toggleGainStationBtn.setVisible(false);
        hideGainStationBtn.setVisible(false);
        loudnessMeter.setVisible(false);
        return;
    }

    int startX = trackInfoWidth;

    if (isGainStationExpanded) {
        toggleGainStationBtn.setVisible(false);
        hideGainStationBtn.setVisible(true);
        loudnessMeter.setVisible(true);

        // Se adhiere perfectamente a la información del track sin gaps
        loudnessMeter.setBounds(trackInfoWidth, 20, gainStationWidth, getHeight() - 30);

        // El botón HIDE se ubica en la esquina superior derecha del área unificada
        hideGainStationBtn.setBounds(trackInfoWidth + gainStationWidth - 45, 5, 40, 15);

        startX += gainStationWidth + padding;
    }
    else {
        toggleGainStationBtn.setVisible(true);
        hideGainStationBtn.setVisible(false);
        loudnessMeter.setVisible(false);

        // La barra negra colapsada se pega a la info del track
        toggleGainStationBtn.setBounds(trackInfoWidth, 0, toggleBarWidth, getHeight());

        startX += toggleBarWidth + padding;
    }

    // --- POSICIONAMIENTO DINÁMICO DE PLUGINS ---
    int x = startX;
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