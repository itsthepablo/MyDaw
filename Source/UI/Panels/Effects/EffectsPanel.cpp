#include "EffectsPanel.h"
#include "UI/EffectDevice.h"
#include "../../../Theme/CustomTheme.h"
#include "../../../Modules/Routing/UI/SendDevice.h"

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
                if (p) {
                    if (!p->getIsInstrument()) {
                        p->setBypassed(isBypassed);
                    }
                }
            }
            updateSlots();
        }
        };

    addAndMakeVisible(addEffectBtn);
    addEffectBtn.setButtonText("+ PLUGIN");
    addEffectBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(70, 75, 80));

    addEffectBtn.onClick = [this] {
        if (activeTrack) {
            juce::PopupMenu m;
            m.addItem(1, "Native: Utility (Gain/Pan)");
            m.addItem(2, "Native: Orion (Synth)");
            m.addSeparator();
            m.addItem(3, "External VST3...");

            m.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
                if (result == 1 && onAddNativeUtility) onAddNativeUtility(*activeTrack);
                if (result == 2 && onAddNativeOrion) onAddNativeOrion(*activeTrack);
                if (result == 3 && onAddVST3) onAddVST3(*activeTrack);
                });
        }
        };

    // --- NUEVO: BOTÓN PARA AÑADIR ENVÍOS ---
    addAndMakeVisible(addSendBtn);
    addSendBtn.setButtonText("+ SEND");
    addSendBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(70, 75, 80));
    addSendBtn.onClick = [this] {
        if (activeTrack && onAddSend) onAddSend(*activeTrack);
    };

    addEffectBtn.setVisible(false);
    addSendBtn.setVisible(false);

    // --- BARRA NEGRA (SOLO VISIBLE CUANDO ESTÁ OCULTO) ---
    addAndMakeVisible(toggleGainStationBtn);
    toggleGainStationBtn.setButtonText(">");
    toggleGainStationBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(15, 18, 20)); 
    toggleGainStationBtn.setTooltip("Mostrar Gain Station");
    toggleGainStationBtn.onClick = [this] {
        isGainStationExpanded = true;
        resized();
        repaint();
        };

    // --- BOTÓN HIDE (SOLO VISIBLE CUANDO ESTÁ EXPANDIDO) ---
    addAndMakeVisible(hideGainStationBtn);
    hideGainStationBtn.setButtonText("HIDE");
    hideGainStationBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    hideGainStationBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white.withAlpha(0.4f));
    hideGainStationBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    hideGainStationBtn.onClick = [this] {
        isGainStationExpanded = false;
        resized();
        repaint();
        };

    addAndMakeVisible(gainStation);
}

void EffectsPanel::setTrack(Track* t) {
    activeTrack = t;
    if (activeTrack) {
        gsBridge = std::make_unique<GainStationBridge>(
            activeTrack->gainStationData, 
            activeTrack->preLoudness, 
            activeTrack->postLoudness
        );
        gainStation.setBridge(gsBridge.get());
    } else {
        gsBridge.reset();
        gainStation.setBridge(nullptr);
    }
    updateSlots();
}

void EffectsPanel::updateSlots() {
    devices.clear();
    sends.clear(); // NUEVO

    addEffectBtn.setVisible(activeTrack != nullptr);
    addSendBtn.setVisible(activeTrack != nullptr);
    bypassAllBtn.setVisible(activeTrack != nullptr);

    bool allBypassed = true;
    bool hasPlugins = false;

    if (activeTrack) {
        // --- 1. PLUGINS ---
        for (int i = 0; i < activeTrack->plugins.size(); ++i) {
            auto* effectRef = activeTrack->plugins[i];
            if (!effectRef) continue;
            if (effectRef->getIsInstrument()) continue;

            hasPlugins = true;
            if (!effectRef->isBypassed()) allBypassed = false;

            juce::String name = "Plugin";
            if (effectRef->isLoaded()) name = effectRef->getLoadedPluginName();
            bool isBypassed = effectRef->isBypassed();

            auto* device = new EffectDevice(i, name, false, isBypassed, effectRef, *this);
            devices.add(device);
            addAndMakeVisible(device);
        }

        // --- 2. ENVÍOS ---
        for (int i = 0; i < (int)activeTrack->routingData.sends.size(); ++i) {
            auto tracks = getAvailableTracks ? getAvailableTracks() : juce::Array<Track*>();
            auto* sendDev = new SendDevice(i, activeTrack, tracks, 
                [this] { if (onChangeSend) onChangeSend(*activeTrack); },
                [this](int idx) { if (onDeleteSend) onDeleteSend(*activeTrack, idx); });
            sends.add(sendDev);
            addAndMakeVisible(sendDev);
        }
    }

    if (hasPlugins && allBypassed) bypassAllBtn.setToggleState(true, juce::dontSendNotification);
    else bypassAllBtn.setToggleState(false, juce::dontSendNotification);

    resized();
    repaint();
}

void EffectsPanel::updateStyles() {
    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
        auto bg = theme->getSkinColor("EFFECTS_BG", juce::Colour(40, 43, 48));
        bypassAllBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
        addEffectBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
        addSendBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
        
        // También actualizar la GainStation si es necesario
        gainStation.repaint();
    }
}

void EffectsPanel::paint(juce::Graphics& g) {
    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
        g.fillAll(theme->getSkinColor("EFFECTS_BG", juce::Colour(40, 43, 48)));
    } else {
        g.fillAll(juce::Colour(40, 43, 48));
    }

    int trackInfoWidth = 110;
    int gainStationWidth = isGainStationExpanded ? gainStation.getPreferredWidth() : 0;
    int totalLeftWidth = trackInfoWidth + gainStationWidth; 

    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
        g.setColour(theme->getSkinColor("EFFECTS_BG", juce::Colour(40, 43, 48)).darker(0.1f));
    } else {
        g.setColour(juce::Colour(30, 33, 36));
    }
    
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
    int gainStationWidth = gainStation.getPreferredWidth();
    int toggleBarWidth = 20;
    int padding = 15;

    bypassAllBtn.setBounds(15, 70, 80, 20);

    if (!activeTrack) {
        toggleGainStationBtn.setVisible(false);
        hideGainStationBtn.setVisible(false);
        gainStation.setVisible(false);
        return;
    }

    int startX = trackInfoWidth;

    if (isGainStationExpanded) {
        toggleGainStationBtn.setVisible(false);
        hideGainStationBtn.setVisible(true);
        gainStation.setVisible(true);
        gainStation.setBounds(trackInfoWidth, 20, gainStationWidth, getHeight() - 30);
        hideGainStationBtn.setBounds(trackInfoWidth + gainStationWidth - 45, 5, 40, 15);
        startX += gainStationWidth + padding;
    }
    else {
        toggleGainStationBtn.setVisible(true);
        hideGainStationBtn.setVisible(false);
        gainStation.setVisible(false);
        toggleGainStationBtn.setBounds(trackInfoWidth, 0, toggleBarWidth, getHeight());
        startX += toggleBarWidth + padding;
    }

    int x = startX;
    int y = 20;
    int slotHeight = getHeight() - 40;
    if (slotHeight < 50) slotHeight = 50;

    // --- 1. PLUGINS ---
    for (auto* device : devices) {
        int dWidth = device->getPreferredWidth();
        device->setBounds(x, y, dWidth, slotHeight);
        x += dWidth + padding;
    }

    addEffectBtn.setBounds(x, y + (slotHeight / 2) - 20, 80, 40);
    x += 100;

    // --- 2. SEPARADOR VISUAL PARA SENDS ---
    if (sends.size() > 0) {
        x += 20;
    }

    // --- 3. SENDS ---
    for (auto* send : sends) {
        send->setBounds(x, y, 130, slotHeight);
        x += 130 + padding;
    }

    addSendBtn.setBounds(x, y + (slotHeight / 2) - 20, 80, 40);
}