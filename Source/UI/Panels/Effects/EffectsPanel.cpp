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
            m.addItem(3, "Native: Node Patcher");
            m.addSeparator();
            m.addItem(4, "External VST3...");

            m.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
                if (result == 1 && onAddNativeUtility) onAddNativeUtility(*activeTrack);
                if (result == 2 && onAddNativeOrion) onAddNativeOrion(*activeTrack);
                if (result == 3 && onAddNativeNodePatcher) onAddNativeNodePatcher(*activeTrack);
                if (result == 4 && onAddVST3) onAddVST3(*activeTrack);
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

    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&container, false);
    viewport.setScrollBarsShown(true, false, false, true);

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
    container.addAndMakeVisible(addEffectBtn);
    container.addAndMakeVisible(addSendBtn);
}

void EffectsPanel::setTrack(Track* t) {
    activeTrack = t;
    if (activeTrack) {
        gsBridge = std::make_unique<GainStationBridge>(
            activeTrack->gainStationData, 
            activeTrack->dsp.preLoudness, 
            activeTrack->dsp.postLoudness
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
            container.addAndMakeVisible(device);
        }

        // --- 2. ENVÍOS ---
        for (int i = 0; i < (int)activeTrack->routingData.sends.size(); ++i) {
            auto tracks = getAvailableTracks ? getAvailableTracks() : juce::Array<Track*>();
            auto* sendDev = new SendDevice(i, activeTrack, tracks, 
                [this] { if (onChangeSend) onChangeSend(*activeTrack); },
                [this](int idx) { if (onDeleteSend) onDeleteSend(*activeTrack, idx); });
            sends.add(sendDev);
            container.addAndMakeVisible(sendDev);
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

    int trackInfoHeight = 110;

    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
        g.setColour(theme->getSkinColor("EFFECTS_BG", juce::Colour(40, 43, 48)).darker(0.1f));
    } else {
        g.setColour(juce::Colour(30, 33, 36));
    }
    
    g.fillRect(0, 0, getWidth(), trackInfoHeight);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawHorizontalLine(trackInfoHeight, 0.0f, (float)getWidth());

    if (!activeTrack) {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText("SELECCIONA PISTA", 0, 0, getWidth(), getHeight(), juce::Justification::centred);
        return;
    }

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("EFFECTS", 15, 10, getWidth() - 30, 30, juce::Justification::centredLeft);

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(juce::Font(14.0f));
    g.drawText(activeTrack->getName(), 35, 40, getWidth() - 50, 20, juce::Justification::centredLeft);

    g.setColour(activeTrack->getColor());
    g.fillEllipse(15, 43, 12, 12);
}

void EffectsPanel::resized() {
    int trackInfoHeight = 110;
    int toggleBarHeight = 20;
    int padding = 15;

    bypassAllBtn.setBounds(15, 70, 80, 20);

    if (!activeTrack) {
        toggleGainStationBtn.setVisible(false);
        hideGainStationBtn.setVisible(false);
        gainStation.setVisible(false);
        viewport.setBounds(0, trackInfoHeight, getWidth(), getHeight() - trackInfoHeight);
        return;
    }

    int startY = trackInfoHeight;
    int slotWidth = getWidth() - 25; // espacio para el scrollbar
    if (slotWidth < 50) slotWidth = 50;

    if (isGainStationExpanded) {
        int gainStationHeight = gainStation.getPreferredHeight();
        toggleGainStationBtn.setVisible(false);
        hideGainStationBtn.setVisible(true);
        gainStation.setVisible(true);
        gainStation.setBounds(10, startY, getWidth() - 20, gainStationHeight);
        hideGainStationBtn.setBounds(getWidth() - 45, startY + 5, 40, 15);
        startY += gainStationHeight + padding;
    }
    else {
        toggleGainStationBtn.setVisible(true);
        hideGainStationBtn.setVisible(false);
        gainStation.setVisible(false);
        toggleGainStationBtn.setButtonText("v");
        toggleGainStationBtn.setBounds(0, startY, getWidth(), toggleBarHeight);
        startY += toggleBarHeight + padding;
    }

    // Set viewport bounds below GainStation
    viewport.setBounds(0, startY, getWidth(), getHeight() - startY);

    int y = 0; // Local al container
    int viewWidth = viewport.getWidth();
    if (viewWidth == 0) viewWidth = getWidth();
    
    int x = 10;

    // --- 1. PLUGINS ---
    for (auto* device : devices) {
        int dHeight = device->getPreferredHeight();
        device->setBounds(x, y, slotWidth, dHeight);
        y += dHeight + padding;
    }

    addEffectBtn.setBounds(x, y, slotWidth, 40);
    y += 40 + padding;

    // --- 2. SEPARADOR VISUAL PARA SENDS ---
    if (sends.size() > 0) {
        y += 20;
    }

    // --- 3. SENDS ---
    for (auto* send : sends) {
        send->setBounds(x, y, slotWidth, 110); 
        y += 110 + padding;
    }

    addSendBtn.setBounds(x, y, slotWidth, 40);
    y += 40 + padding;

    container.setSize(viewWidth, y);
}