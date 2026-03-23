#pragma once
#include <JuceHeader.h>
#include "Track.h"
#include "../PluginHost/VSTHost.h" 
#include "../UI/Knobs/FLKnobLookAndFeel.h"
#include "../UI/Knobs/FloatingValueSlider.h" 
#include "../UI/LevelMeter.h"

class TrackControlPanel : public juce::Component, private juce::Timer {
public:
    std::function<void()> onFxClick, onPianoRollClick, onDeleteClick, onEffectsClick, onFolderStateChange;
    std::function<void()> onWaveformViewChanged;
    std::function<void(int)> onPluginClick;

    TrackControlPanel(Track& t) : track(t) {
        addAndMakeVisible(nameLabel);
        nameLabel.setText(track.getName(), juce::dontSendNotification);
        nameLabel.setEditable(true);
        nameLabel.setTooltip("Nombre de Pista: Haz doble clic para renombrar."); // TEXTO NUEVO
        nameLabel.onTextChange = [this] { track.setName(nameLabel.getText()); };

        // --- BOTONES DE MUTE Y SOLO ---
        addAndMakeVisible(muteBtn);
        muteBtn.setButtonText("M");
        muteBtn.setClickingTogglesState(true);
        muteBtn.setToggleState(track.isMuted, juce::dontSendNotification);
        muteBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 52));
        muteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker(0.2f));
        muteBtn.setTooltip("Mute: Silencia el audio de esta pista."); // TEXTO NUEVO
        muteBtn.onClick = [this] { track.isMuted = muteBtn.getToggleState(); };

        addAndMakeVisible(soloBtn);
        soloBtn.setButtonText("S");
        soloBtn.setClickingTogglesState(true);
        soloBtn.setToggleState(track.isSoloed, juce::dontSendNotification);
        soloBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 52));
        soloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow.darker(0.2f));
        soloBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::black); 
        soloBtn.setTooltip("Solo: Reproduce solo esta pista silenciando las demás."); // TEXTO NUEVO
        soloBtn.onClick = [this] { track.isSoloed = soloBtn.getToggleState(); };

        // --- KNOBS PROFESIONALES ---
        addAndMakeVisible(volKnob); addAndMakeVisible(panKnob);
        
        volKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        volKnob.setLookAndFeel(&flLookAndFeel); 
        volKnob.setRange(0.0, 1.0); 
        volKnob.setValue(track.getVolume(), juce::dontSendNotification);
        volKnob.setTooltip("Volumen: Ajusta la ganancia principal del canal."); // TEXTO NUEVO
        volKnob.onValueChange = [this] { track.setVolume((float)volKnob.getValue()); };

        volKnob.valueToTextFormattingCallback = [](double rawGain) -> juce::String {
            float db = juce::Decibels::gainToDecibels((float)rawGain, -100.0f);
            if (db <= -100.0f) return "-inf dB";
            return juce::String(db, 1) + " dB"; 
        };

        panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panKnob.setLookAndFeel(&flLookAndFeel); 
        panKnob.setRange(-1.0, 1.0); 
        panKnob.setValue(track.getBalance(), juce::dontSendNotification);
        panKnob.setTooltip("Paneo / Balance: Ajusta la posición estéreo L/R."); // TEXTO NUEVO
        panKnob.onValueChange = [this] { track.setBalance((float)panKnob.getValue()); };

        panKnob.valueToTextFormattingCallback = [](double value) -> juce::String {
            if (std::abs(value) < 0.01) return "Center"; 
            int percent = (int)(std::abs(value) * 100.0);
            return juce::String(percent) + (value < 0.0 ? "% L" : "% R"); 
        };

        addAndMakeVisible(effectsBtn);
        effectsBtn.setButtonText("Effects");
        effectsBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 65, 70));
        effectsBtn.setTooltip("Efectos: Muestra el rack de plugins insertados."); // TEXTO NUEVO
        effectsBtn.onClick = [this] { if (onEffectsClick) onEffectsClick(); };

        addAndMakeVisible(folderBtn);
        folderBtn.setTooltip("Carpeta: Agrupa las pistas inferiores dentro de esta."); // TEXTO NUEVO
        addAndMakeVisible(compactBtn);
        compactBtn.setTooltip("Expandir/Contraer las pistas hijas."); // TEXTO NUEVO

        folderBtn.onClick = [this] {
            if (track.folderDepthChange == 0) track.folderDepthChange = 1;
            else if (track.folderDepthChange > 0) track.folderDepthChange = -1;
            else track.folderDepthChange = 0;

            updateFolderBtnVisuals();
            if (onFolderStateChange) onFolderStateChange();
            };

        compactBtn.onClick = [this] {
            track.isCollapsed = !track.isCollapsed;
            updateFolderBtnVisuals();
            if (onFolderStateChange) onFolderStateChange();
            };

        if (track.getType() == TrackType::MIDI) {
            addAndMakeVisible(prButton); prButton.setButtonText("PIANO ROLL");
            prButton.setTooltip("Piano Roll: Abre el editor MIDI principal."); // TEXTO NUEVO
            prButton.onClick = [this] { if (onPianoRollClick) onPianoRollClick(); };

            addAndMakeVisible(inlineBtn);
            inlineBtn.setButtonText("INLINE");
            inlineBtn.setClickingTogglesState(true);
            inlineBtn.setToggleState(track.isInlineEditingActive, juce::dontSendNotification);
            inlineBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
            inlineBtn.setTooltip("Inline Editor: Edita notas directamente en la Playlist."); // TEXTO NUEVO
            inlineBtn.onClick = [this] {
                track.isInlineEditingActive = inlineBtn.getToggleState();
                if (onWaveformViewChanged) onWaveformViewChanged(); 
                };

            addAndMakeVisible(fxButton); fxButton.setButtonText("+ VSTi");
            fxButton.setTooltip("Añadir Instrumento VSTi a esta pista."); // TEXTO NUEVO
            fxButton.onClick = [this] { if (onFxClick) onFxClick(); };
        }

        addAndMakeVisible(levelMeter);
        levelMeter.setTooltip("Medidor de Nivel: Muestra la salida de audio. Clic para reiniciar pico."); // TEXTO NUEVO
        startTimerHz(30); 

        updateFolderBtnVisuals();
    }

    void timerCallback() override {
        if (track.isMuted) {
            levelMeter.setLevel(0.0f, 0.0f);
            return;
        }

        float vol = track.getVolume();
        float pan = track.getBalance();
        float leftGain = vol * (pan < 0.0f ? 1.0f : 1.0f - pan);
        float rightGain = vol * (pan > 0.0f ? 1.0f : 1.0f + pan);
        levelMeter.setLevel(track.currentPeakLevelL * leftGain, track.currentPeakLevelR * rightGain);
    }

    void updateFolderBtnVisuals() {
        if (track.folderDepthChange > 0) {
            folderBtn.setButtonText("P");
            folderBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::orange);
            compactBtn.setVisible(true);
            compactBtn.setButtonText(track.isCollapsed ? ">" : "v");
        }
        else if (track.folderDepthChange < 0) {
            folderBtn.setButtonText("L");
            folderBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::dodgerblue);
            compactBtn.setVisible(false);
        }
        else {
            folderBtn.setButtonText("O");
            folderBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 45, 50));
            compactBtn.setVisible(false);
        }
    }

    void updatePlugins() {
        pluginButtons.clear();
        for (int i = 0; i < track.plugins.size(); ++i) {
            juce::String name = "VST3";
            if (track.plugins[i] != nullptr && track.plugins[i]->isLoaded()) name = track.plugins[i]->getLoadedPluginName();
            auto* btn = new juce::TextButton(name);
            btn->setTooltip("Abre la interfaz del plugin insertado."); // TEXTO NUEVO
            btn->onClick = [this, i] { if (onPluginClick) onPluginClick(i); };
            pluginButtons.add(btn); addAndMakeVisible(btn);
        }
        resized();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(30, 32, 35));
        int indent = track.folderDepth * 20;
        auto area = getLocalBounds().reduced(2).withTrimmedLeft(indent);
        g.setColour(juce::Colour(40, 42, 46));
        g.fillRoundedRectangle(area.toFloat(), 4.0f);
        g.setColour(track.getColor());
        g.fillRect(area.removeFromLeft(6));
    }

    void resized() override {
        int indent = track.folderDepth * 20;
        auto area = getLocalBounds().reduced(6).withTrimmedLeft(indent);
        
        auto leftCol = area.removeFromLeft(125);
        auto topRow = leftCol.removeFromTop(20);
        if (track.folderDepthChange > 0) compactBtn.setBounds(topRow.removeFromLeft(20).reduced(2));
        nameLabel.setBounds(topRow);
        leftCol.removeFromTop(2);
        
        auto kRow = leftCol.removeFromTop(30);
        
        muteBtn.setBounds(kRow.removeFromLeft(20).reduced(2));
        soloBtn.setBounds(kRow.removeFromLeft(20).reduced(2));
        panKnob.setBounds(kRow.removeFromLeft(35));
        volKnob.setBounds(kRow.removeFromLeft(35));
        
        leftCol.removeFromTop(2);

        if (track.getType() == TrackType::MIDI) {
            auto prRow = leftCol.removeFromTop(18);
            prButton.setBounds(prRow.removeFromLeft(prRow.getWidth() / 2).reduced(2, 0));
            inlineBtn.setBounds(prRow.reduced(2, 0));
            leftCol.removeFromTop(2);
        }

        auto botRow = leftCol.removeFromTop(18);
        folderBtn.setBounds(botRow.removeFromLeft(18));
        effectsBtn.setBounds(botRow.withTrimmedLeft(4));
        
        auto meterArea = area.removeFromRight(10); 
        levelMeter.setBounds(meterArea.reduced(0, 4)); 
        area.removeFromRight(5); 

        auto fxArea = area.removeFromRight(120);

        if (track.getType() == TrackType::MIDI) {
            fxButton.setBounds(fxArea.removeFromTop(18).reduced(2, 0));
            fxArea.removeFromTop(2);
        }

        for (auto* b : pluginButtons) {
            b->setBounds(fxArea.removeFromTop(18).reduced(2, 0));
            fxArea.removeFromTop(2);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isRightButtonDown()) {
            juce::PopupMenu m;
            m.addItem(1, "Eliminar Pista");

            if (track.getType() == TrackType::Audio) {
                m.addSeparator();
                m.addItem(2, "Vista: Combinada (Mono)", true, track.getWaveformViewMode() == WaveformViewMode::Combined);
                m.addItem(3, "Vista: Separada (L / R)", true, track.getWaveformViewMode() == WaveformViewMode::SeparateLR);
                m.addItem(4, "Vista: Mid / Side", true, track.getWaveformViewMode() == WaveformViewMode::MidSide);
            }

            m.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
                if (result == 1 && onDeleteClick) onDeleteClick();
                else if (result == 2) { track.setWaveformViewMode(WaveformViewMode::Combined); if (onWaveformViewChanged) onWaveformViewChanged(); }
                else if (result == 3) { track.setWaveformViewMode(WaveformViewMode::SeparateLR); if (onWaveformViewChanged) onWaveformViewChanged(); }
                else if (result == 4) { track.setWaveformViewMode(WaveformViewMode::MidSide); if (onWaveformViewChanged) onWaveformViewChanged(); }
                });
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (auto* dragC = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
            dragC->startDragging("TRACK", this, juce::Image(), true);
        }
    }

    Track& track;
private:
    juce::Label nameLabel;
    juce::TextButton muteBtn, soloBtn;
    FloatingValueSlider volKnob, panKnob; 
    juce::TextButton fxButton, prButton, inlineBtn, effectsBtn, folderBtn, compactBtn;
    juce::OwnedArray<juce::TextButton> pluginButtons;
    
    FLKnobLookAndFeel flLookAndFeel; 
    LevelMeter levelMeter; 
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackControlPanel)
};