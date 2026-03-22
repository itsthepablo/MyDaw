#pragma once
#include <JuceHeader.h>
#include "PickerPanel.h"
#include "../Effects/EffectsPanel.h"
#include "FileBrowserPanel.h" // NUEVO INCLUDE

class LeftSidebar : public juce::Component {
public:
    enum Tab { PickerTab, FilesTab, FxTab }; // Añadida nueva pestaña

    LeftSidebar(PickerPanel& picker, EffectsPanel& fx, FileBrowserPanel& files)
        : pickerPanel(picker), effectsPanel(fx), filesPanel(files)
    {
        addAndMakeVisible(pickerPanel);
        addAndMakeVisible(effectsPanel);
        addAndMakeVisible(filesPanel); // Añadimos el nuevo panel

        // --- BOTÓN PESTAÑA: PICKER ---
        addAndMakeVisible(pickerTabBtn);
        pickerTabBtn.setButtonText("PICKER");
        pickerTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        pickerTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        pickerTabBtn.setClickingTogglesState(true);
        pickerTabBtn.setRadioGroupId(200); 
        pickerTabBtn.onClick = [this] { showTab(PickerTab); };

        // --- BOTÓN PESTAÑA: FILES (NUEVO) ---
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

        showTab(FilesTab); // Por defecto abriremos en Files porque es el más usado
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

        // Dividimos el espacio inferior entre los tres botones
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
    FileBrowserPanel& filesPanel; // Referencia al nuevo panel
    
    juce::TextButton pickerTabBtn, filesTabBtn, fxTabBtn;
    Tab currentTab = FilesTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftSidebar)
};