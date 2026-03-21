#pragma once
#include <JuceHeader.h>
#include "Track.h"
#include "../PluginHost/VSTHost.h" 

class TrackControlPanel : public juce::Component {
public:
    std::function<void()> onFxClick, onPianoRollClick, onDeleteClick, onEffectsClick, onFolderStateChange;
    std::function<void(int)> onPluginClick;

    TrackControlPanel(Track& t) : track(t) {
        addAndMakeVisible(nameLabel);
        nameLabel.setText(track.getName(), juce::dontSendNotification);
        nameLabel.setEditable(true);
        nameLabel.onTextChange = [this] { track.setName(nameLabel.getText()); };

        addAndMakeVisible(volSlider); addAndMakeVisible(panSlider);
        volSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        volSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        volSlider.onValueChange = [this] { track.setVolume((float)volSlider.getValue()); };

        panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panSlider.setRange(-1.0, 1.0);
        panSlider.onValueChange = [this] { track.setBalance((float)panSlider.getValue()); };

        addAndMakeVisible(effectsBtn);
        effectsBtn.setButtonText("Effects");
        effectsBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 65, 70));
        effectsBtn.onClick = [this] { if (onEffectsClick) onEffectsClick(); };

        addAndMakeVisible(folderBtn);
        addAndMakeVisible(compactBtn);

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
            prButton.onClick = [this] { if (onPianoRollClick) onPianoRollClick(); };

            addAndMakeVisible(fxButton); fxButton.setButtonText("+ VSTi");
            fxButton.onClick = [this] { if (onFxClick) onFxClick(); };
        }

        updateFolderBtnVisuals();
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
            btn->onClick = [this, i] { if (onPluginClick) onPluginClick(i); };
            pluginButtons.add(btn); addAndMakeVisible(btn);
        }
        resized();
    }

    void paint(juce::Graphics& g) override {
        // Fondo general (Ocupa todo el ancho)
        g.fillAll(juce::Colour(30, 32, 35));

        int indent = track.folderDepth * 20;
        auto area = getLocalBounds().reduced(2).withTrimmedLeft(indent);

        // Fondo del panel desplazado
        g.setColour(juce::Colour(40, 42, 46));
        g.fillRoundedRectangle(area.toFloat(), 4.0f);

        // Franja de color desplazada
        g.setColour(track.getColor());
        g.fillRect(area.removeFromLeft(6));
    }

    void resized() override {
        // La matemática crítica: Cortamos el espacio a la izquierda según la profundidad
        int indent = track.folderDepth * 20;
        auto area = getLocalBounds().reduced(6).withTrimmedLeft(indent);

        // A partir de aquí, todo se dibuja dentro del espacio ya desplazado
        auto leftCol = area.removeFromLeft(125);

        auto topRow = leftCol.removeFromTop(20);
        if (track.folderDepthChange > 0) compactBtn.setBounds(topRow.removeFromLeft(20).reduced(2));
        nameLabel.setBounds(topRow);

        leftCol.removeFromTop(2);

        auto kRow = leftCol.removeFromTop(30);
        volSlider.setBounds(kRow.removeFromLeft(35));
        panSlider.setBounds(kRow.removeFromLeft(35));

        leftCol.removeFromTop(2);

        if (track.getType() == TrackType::MIDI) {
            prButton.setBounds(leftCol.removeFromTop(18).reduced(2, 0));
            leftCol.removeFromTop(2);
        }

        auto botRow = leftCol.removeFromTop(18);
        folderBtn.setBounds(botRow.removeFromLeft(18));
        effectsBtn.setBounds(botRow.withTrimmedLeft(4));

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
            juce::PopupMenu m; m.addItem(1, "Eliminar Pista");
            m.showMenuAsync(juce::PopupMenu::Options(), [this](int result) { if (result == 1 && onDeleteClick) onDeleteClick(); });
        }
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        if (auto* dragC = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
            dragC->startDragging("TRACK", this, juce::Image(), true);
        }
    }

    Track& track;
private:
    juce::Label nameLabel; juce::Slider volSlider, panSlider;
    juce::TextButton fxButton, prButton, effectsBtn, folderBtn, compactBtn;
    juce::OwnedArray<juce::TextButton> pluginButtons;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackControlPanel)
};