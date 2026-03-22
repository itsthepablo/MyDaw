#pragma once
#include <JuceHeader.h>
#include "PickerPanel.h"
#include "../Effects/EffectsPanel.h"
#include "FileBrowserPanel.h" 

class LeftSidebar : public juce::Component {
public:
    enum Tab { PickerTab, FilesTab, FxTab };

    std::function<void()> onClose; // Callback para avisar que queremos cerrar el panel

    LeftSidebar(PickerPanel& picker, EffectsPanel& fx, FileBrowserPanel& files)
        : pickerPanel(picker), effectsPanel(fx), filesPanel(files)
    {
        addAndMakeVisible(pickerPanel);
        addAndMakeVisible(effectsPanel);
        addAndMakeVisible(filesPanel);

        // --- BOTÓN PESTAÑA: PICKER ---
        addAndMakeVisible(pickerTabBtn);
        pickerTabBtn.setButtonText("PICKER");
        pickerTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        pickerTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        pickerTabBtn.setClickingTogglesState(true);
        pickerTabBtn.setRadioGroupId(200);
        pickerTabBtn.onClick = [this] { showTab(PickerTab); };

        // --- BOTÓN PESTAÑA: FILES ---
        addAndMakeVisible(filesTabBtn);
        filesTabBtn.setButtonText("FILES");
        filesTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        filesTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        filesTabBtn.setClickingTogglesState(true);
        filesTabBtn.setRadioGroupId(200);
        filesTabBtn.onClick = [this] { showTab(FilesTab); };

        // --- BOTÓN PESTAÑA: EFFECTS ---
        addAndMakeVisible(fxTabBtn);
        fxTabBtn.setButtonText("EFFECTS");
        fxTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        fxTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        fxTabBtn.setClickingTogglesState(true);
        fxTabBtn.setRadioGroupId(200);
        fxTabBtn.onClick = [this] { showTab(FxTab); };

        // --- BOTÓN CERRAR [X] ---
        addAndMakeVisible(closeBtn);
        closeBtn.setButtonText("X");
        closeBtn.setTooltip("Cerrar Panel Lateral");
        closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        closeBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
        closeBtn.onClick = [this] { if (onClose) onClose(); };

        showTab(FilesTab);
    }

    void showTab(Tab tab) {
        currentTab = tab;

        pickerPanel.setVisible(tab == PickerTab);
        filesPanel.setVisible(tab == FilesTab);
        effectsPanel.setVisible(tab == FxTab);

        if (tab == PickerTab) pickerTabBtn.setToggleState(true, juce::dontSendNotification);
        else if (tab == FilesTab) filesTabBtn.setToggleState(true, juce::dontSendNotification);
        else fxTabBtn.setToggleState(true, juce::dontSendNotification);

        resized();
    }

    Tab getCurrentTab() const { return currentTab; }

    void resized() override {
        auto area = getLocalBounds();
        auto tabArea = area.removeFromBottom(30);

        // Posicionamos la [X] en el extremo derecho
        closeBtn.setBounds(tabArea.removeFromRight(30).reduced(2));

        // Dividimos el espacio restante entre los tres botones de pestaña
        int tabW = tabArea.getWidth() / 3;
        pickerTabBtn.setBounds(tabArea.removeFromLeft(tabW));
        filesTabBtn.setBounds(tabArea.removeFromLeft(tabW));
        fxTabBtn.setBounds(tabArea);

        if (currentTab == PickerTab) pickerPanel.setBounds(area);
        else if (currentTab == FilesTab) filesPanel.setBounds(area);
        else effectsPanel.setBounds(area);
    }

private:
    PickerPanel& pickerPanel;
    EffectsPanel& effectsPanel;
    FileBrowserPanel& filesPanel;

    juce::TextButton pickerTabBtn, filesTabBtn, fxTabBtn, closeBtn; // Añadido closeBtn
    Tab currentTab = FilesTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftSidebar)
};