#pragma once
#include <JuceHeader.h>
#include "Track.h"
#include "../PluginHost/VSTHost.h" 
#include "../UI/Knobs/FloatingValueSlider.h" 
#include "../UI/LevelMeter.h"

class TrackControlPanel : public juce::Component, private juce::Timer, public juce::ChangeListener {
public:
    std::function<void()> onFxClick, onInstrumentClick, onPianoRollClick, onDeleteClick, onEffectsClick, onFolderStateChange;
    std::function<void()> onWaveformViewChanged, onTrackColorChanged;
    std::function<void(int)> onPluginClick;
    std::function<void(const juce::ModifierKeys&)> onTrackSelected;
    std::function<void()> onMoveToNewFolder;
    std::function<void(int, juce::String)> onCreateAutomation;

    int dragHoverMode = 0;

    TrackControlPanel(Track& t) : track(t) {
        addAndMakeVisible(nameLabel);
        nameLabel.setText(track.getName(), juce::dontSendNotification);
        nameLabel.setEditable(true);
        nameLabel.onTextChange = [this] { track.setName(nameLabel.getText()); };

        addAndMakeVisible(muteBtn);
        muteBtn.setButtonText("M");
        muteBtn.setClickingTogglesState(true);
        muteBtn.setToggleState(track.isMuted, juce::dontSendNotification);
        muteBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 52));
        muteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker(0.2f));
        muteBtn.onClick = [this] { track.isMuted = muteBtn.getToggleState(); };

        addAndMakeVisible(soloBtn);
        soloBtn.setButtonText("S");
        soloBtn.setClickingTogglesState(true);
        soloBtn.setToggleState(track.isSoloed, juce::dontSendNotification);
        soloBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 52));
        soloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow.darker(0.2f));
        soloBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        soloBtn.onClick = [this] { track.isSoloed = soloBtn.getToggleState(); };

        addAndMakeVisible(volKnob); addAndMakeVisible(panKnob);
        volKnob.setName("TrackKnob");
        volKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        volKnob.setRange(0.0, 1.0);
        volKnob.setValue(track.getVolume(), juce::dontSendNotification);
        volKnob.onValueChange = [this] { track.setVolume((float)volKnob.getValue()); };

        volKnob.valueToTextFormattingCallback = [](double rawGain) -> juce::String {
            float db = juce::Decibels::gainToDecibels((float)rawGain, -100.0f);
            return db <= -100.0f ? "-inf dB" : juce::String(db, 1) + " dB";
        };

        panKnob.setName("TrackKnob");
        panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panKnob.setRange(-1.0, 1.0);
        panKnob.setValue(track.getBalance(), juce::dontSendNotification);
        panKnob.onValueChange = [this] { track.setBalance((float)panKnob.getValue()); };

        panKnob.valueToTextFormattingCallback = [](double value) -> juce::String {
            if (std::abs(value) < 0.01) return "Center";
            int percent = (int)(std::abs(value) * 100.0);
            return juce::String(percent) + (value < 0.0 ? "% L" : "% R");
        };

        volKnob.addMouseListener(this, false);
        panKnob.addMouseListener(this, false);

        addAndMakeVisible(effectsBtn);
        effectsBtn.setButtonText("Effects");
        effectsBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 65, 70));
        effectsBtn.onClick = [this] { if (onEffectsClick) onEffectsClick(); };

        addAndMakeVisible(folderBtn);
        addAndMakeVisible(compactBtn);

        folderBtn.onClick = [this] {
            if (track.getType() == TrackType::Folder) {
                track.isCollapsed = !track.isCollapsed;
                updateFolderBtnVisuals();
                if (onFolderStateChange) onFolderStateChange();
            }
        };

        if (track.getType() == TrackType::MIDI) {
            addAndMakeVisible(prButton); prButton.setButtonText("PIANO ROLL");
            prButton.onClick = [this] { if (onPianoRollClick) onPianoRollClick(); };

            addAndMakeVisible(inlineBtn);
            inlineBtn.setButtonText("INLINE");
            inlineBtn.setClickingTogglesState(true);
            inlineBtn.setToggleState(track.isInlineEditingActive, juce::dontSendNotification);
            inlineBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
            inlineBtn.onClick = [this] {
                track.isInlineEditingActive = inlineBtn.getToggleState();
                if (onWaveformViewChanged) onWaveformViewChanged();
            };

            addAndMakeVisible(fxButton); fxButton.setButtonText("+ VSTi");
            fxButton.onClick = [this] { if (onInstrumentClick) onInstrumentClick(); };
        }

        addAndMakeVisible(levelMeter);
        startTimerHz(30);
        setWantsKeyboardFocus(true);
        updateFolderBtnVisuals();
    }

    ~TrackControlPanel() { stopTimer(); }

    void timerCallback() override {
        if (track.isMuted) { levelMeter.setLevel(0.0f, 0.0f); return; }
        float vol = track.getVolume();
        float pan = track.getBalance();
        float leftGain = vol * (pan < 0.0f ? 1.0f : 1.0f - pan);
        float rightGain = vol * (pan > 0.0f ? 1.0f : 1.0f + pan);
        levelMeter.setLevel(track.currentPeakLevelL * leftGain, track.currentPeakLevelR * rightGain);
    }

    void updateFolderBtnVisuals() {
        if (track.getType() == TrackType::Folder) {
            folderBtn.setButtonText(track.isCollapsed ? "+" : "-");
            folderBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 85, 95));
            folderBtn.setVisible(true);
        } else { folderBtn.setVisible(false); }
        compactBtn.setVisible(false);
    }

    void updatePlugins() {
        pluginButtons.clear();
        for (int i = 0; i < track.plugins.size(); ++i) {
            juce::String name = "VST3";
            if (track.plugins[i] != nullptr && track.plugins[i]->isLoaded()) name = track.plugins[i]->getLoadedPluginName();
            auto* btn = new juce::TextButton(name);
            btn->onClick = [this, i] { if (onPluginClick) onPluginClick(i); };
            pluginButtons.add(btn); addAndMakeVisible(btn);
        }
        resized();
    }

    void paint(juce::Graphics& g) override {
        // GRID FIX: Fondo sólido sin huecos
        g.fillAll(juce::Colour(30, 32, 35));
        auto area = getLocalBounds();
        int indent = track.folderDepth * 15;
        
        // --- NUEVO DISEÑO: Indicador Desacoplado ---
        auto colorBarArea = area.withTrimmedLeft(indent + 4).removeFromLeft(6).reduced(0, 4);
        g.setColour(track.getColor());
        g.fillRoundedRectangle(colorBarArea.toFloat(), 3.0f);

        // --- Fondo de la pista (con Gap tras el indicador) ---
        auto contentArea = area.withTrimmedLeft(indent + 4 + 6 + 4); // Indent + Margen + AnchoBarra + Gap
        g.setColour(track.getColor().withAlpha(0.15f));
        g.fillRoundedRectangle(contentArea.toFloat().reduced(0, 2), 4.0f);

        // GRID FIX: Líneas de jerarquía milimétricas
        if (track.folderDepth > 0) {
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            for (int i = 1; i <= track.folderDepth; ++i) {
                float lineX = (float)((i * 15) - 7.5f);
                g.drawVerticalLine((int)lineX, 2.0f, (float)getHeight() - 2.0f);
                if (i == track.folderDepth) g.drawHorizontalLine(getHeight() / 2, lineX, (float)indent);
            }
        }

        if (track.getType() == TrackType::Folder) {
            auto iconArea = contentArea.removeFromTop(20).removeFromLeft(20).reduced(5);
            g.setColour(juce::Colours::orange.withAlpha(0.6f));
            g.fillRect(iconArea);
        }

        if (track.isSelected) {
            g.setColour(juce::Colours::white.withAlpha(0.12f));
            g.fillRoundedRectangle(contentArea.toFloat().reduced(0, 2), 4.0f);
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.drawRoundedRectangle(contentArea.toFloat().reduced(0, 2), 4.0f, 1.5f);
        }

        if (dragHoverMode != 0) {
            g.setColour(dragHoverMode == 2 ? juce::Colours::cyan : juce::Colours::white);
            if (dragHoverMode == 1) g.fillRect(0, 0, getWidth(), 2);
            else if (dragHoverMode == 3) g.fillRect(indent, getHeight() - 2, getWidth() - indent, 2);
            else if (dragHoverMode == 2) g.drawRect(contentArea, 2);
        }
    }

    void resized() override {
        int indent = track.folderDepth * 15;
        // GRID FIX: Altura estricta de 100px
        auto area = getLocalBounds();
        // El contenido real empieza después del indicador (Indent + Margen + Barra + Gap)
        auto contentArea = area.withTrimmedLeft(indent + 4 + 6 + 4).reduced(4, 2);

        auto leftCol = contentArea.removeFromLeft(125);
        auto topRow = leftCol.removeFromTop(20);
        if (track.getType() == TrackType::Folder) folderBtn.setBounds(topRow.removeFromLeft(20).reduced(2));
        nameLabel.setBounds(topRow);
        
        leftCol.removeFromTop(2);
        auto kRow = leftCol.removeFromTop(28);
        muteBtn.setBounds(kRow.removeFromLeft(20).reduced(1));
        soloBtn.setBounds(kRow.removeFromLeft(20).reduced(1));
        panKnob.setBounds(kRow.removeFromLeft(35));
        volKnob.setBounds(kRow.removeFromLeft(35));

        leftCol.removeFromTop(4);
        if (track.getType() == TrackType::MIDI) {
            auto prRow = leftCol.removeFromTop(18);
            prButton.setBounds(prRow.removeFromLeft(prRow.getWidth() / 2).reduced(1));
            inlineBtn.setBounds(prRow.reduced(1));
            leftCol.removeFromTop(4);
        }

        auto botRow = leftCol.removeFromTop(18);
        effectsBtn.setBounds(botRow.reduced(2, 0));

        auto meterArea = contentArea.removeFromRight(12);
        levelMeter.setBounds(meterArea.reduced(0, 2));
        contentArea.removeFromRight(8);

        auto fxArea = contentArea.removeFromRight(110);
        if (track.getType() == TrackType::MIDI) {
            fxButton.setBounds(fxArea.removeFromTop(18).reduced(1));
            fxArea.removeFromTop(2);
        }
        for (auto* b : pluginButtons) {
            b->setBounds(fxArea.removeFromTop(18).reduced(1));
            fxArea.removeFromTop(2);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        grabKeyboardFocus();
        if (e.mods.isPopupMenu()) {
            if (e.originalComponent == &volKnob) {
                showAutomationMenu(0, "Volume");
                return;
            } else if (e.originalComponent == &panKnob) {
                showAutomationMenu(1, "Pan");
                return;
            }
        }

        if (e.mods.isLeftButtonDown() && onTrackSelected) onTrackSelected(e.mods);
        if (e.mods.isRightButtonDown()) {
            juce::PopupMenu m;
            m.addItem(1, "Eliminar Pista");
            m.addItem(7, "Move to New Routing Folder", onMoveToNewFolder != nullptr);
            m.addSeparator();
            m.addItem(6, "Cambiar Color...");
            if (track.getType() == TrackType::Audio) {
                m.addSeparator();
                m.addItem(2, "Vista: Combinada (L+R y Polaridad)", true, track.getWaveformViewMode() == WaveformViewMode::Combined);
                m.addItem(3, "Vista: Separada (L / R)", true, track.getWaveformViewMode() == WaveformViewMode::SeparateLR);
                m.addItem(4, "Vista: Mid / Side", true, track.getWaveformViewMode() == WaveformViewMode::MidSide);
                m.addItem(5, "Vista: Espectrograma (Frecuencias)", true, track.getWaveformViewMode() == WaveformViewMode::Spectrogram);
            }
            m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) {
                if (r == 1 && onDeleteClick) onDeleteClick();
                else if (r == 7 && onMoveToNewFolder) onMoveToNewFolder();
                else if (r == 6) {
                    auto* cs = new juce::ColourSelector(juce::ColourSelector::showColourAtTop | juce::ColourSelector::showSliders | juce::ColourSelector::showColourspace);
                    cs->setCurrentColour(track.getColor()); cs->addChangeListener(this); cs->setSize(300, 400);
                    juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(cs), getScreenBounds(), nullptr);
                } else if (r >= 2 && r <= 5 && onWaveformViewChanged) { track.setWaveformViewMode((WaveformViewMode)(r-2)); onWaveformViewChanged(); }
            });
        }
    }

    void changeListenerCallback(juce::ChangeBroadcaster* s) override {
        if (auto* cs = dynamic_cast<juce::ColourSelector*>(s)) {
            track.setColor(cs->getCurrentColour()); repaint(); if (onTrackColorChanged) onTrackColorChanged();
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        // --- FIX: Si el arrastre empezó en un mando o botón, NO arrastramos la pista ---
        if (e.originalComponent == &volKnob || e.originalComponent == &panKnob || 
            e.originalComponent == &muteBtn || e.originalComponent == &soloBtn ||
            e.originalComponent == &fxButton || e.originalComponent == &prButton ||
            e.originalComponent == &inlineBtn || e.originalComponent == &effectsBtn)
            return;

        if (auto* dc = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
            juce::Image g(juce::Image::ARGB, 1, 1, true); dc->startDragging("TRACK", this, g, true);
        }
    }

    bool keyPressed(const juce::KeyPress& k) override {
        if (k.getKeyCode() == juce::KeyPress::deleteKey || k.getKeyCode() == juce::KeyPress::backspaceKey) { if (onDeleteClick) onDeleteClick(); return true; }
        return false;
    }

    Track& track;
private:
    void showAutomationMenu(int paramId, const juce::String& paramName) {
        juce::PopupMenu m;
        m.addItem(1, "Create Automation Clip");
        m.showMenuAsync(juce::PopupMenu::Options(), [this, paramId, paramName](int res) {
            if (res == 1 && onCreateAutomation) {
                onCreateAutomation(paramId, paramName);
            }
        });
    }

    juce::Label nameLabel; juce::TextButton muteBtn, soloBtn; FloatingValueSlider volKnob, panKnob;
    juce::TextButton fxButton, prButton, inlineBtn, effectsBtn, folderBtn, compactBtn;
    juce::OwnedArray<juce::TextButton> pluginButtons; LevelMeter levelMeter;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackControlPanel)
};