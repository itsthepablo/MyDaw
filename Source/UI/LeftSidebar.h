#pragma once
#include <JuceHeader.h>
#include "PickerPanel.h"
#include "../Effects/EffectsPanel.h"

class LeftSidebar : public juce::Component {
public:
    enum Tab { PickerTab, FxTab };

    LeftSidebar(PickerPanel& picker, EffectsPanel& fx)
        : pickerPanel(picker), effectsPanel(fx)
    {
        // Añadimos los paneles que nos pasa el MainComponent
        addAndMakeVisible(pickerPanel);
        addAndMakeVisible(effectsPanel);

        // --- BOTÓN PESTAÑA: PICKER ---
        addAndMakeVisible(pickerTabBtn);
        pickerTabBtn.setButtonText("PICKER");
        pickerTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        pickerTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        pickerTabBtn.setClickingTogglesState(true);
        pickerTabBtn.setRadioGroupId(200); // Mismo grupo para que actúen como Tabs
        pickerTabBtn.onClick = [this] { showTab(PickerTab); };

        // --- BOTÓN PESTAÑA: EFFECTS ---
        addAndMakeVisible(fxTabBtn);
        fxTabBtn.setButtonText("EFFECTS");
        fxTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        fxTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        fxTabBtn.setClickingTogglesState(true);
        fxTabBtn.setRadioGroupId(200);
        fxTabBtn.onClick = [this] { showTab(FxTab); };

        showTab(PickerTab); // Mostrar Picker por defecto
    }

    void showTab(Tab tab) {
        currentTab = tab;
        if (tab == PickerTab) {
            pickerTabBtn.setToggleState(true, juce::dontSendNotification);
            pickerPanel.setVisible(true);
            effectsPanel.setVisible(false);
        } else {
            fxTabBtn.setToggleState(true, juce::dontSendNotification);
            pickerPanel.setVisible(false);
            effectsPanel.setVisible(true);
        }
        resized();
    }

    Tab getCurrentTab() const { return currentTab; }

    void resized() override {
        auto area = getLocalBounds();
        
        // Reservamos 30 píxeles abajo para las pestañas
        auto tabArea = area.removeFromBottom(30);

        // Dividimos el espacio inferior entre los dos botones
        pickerTabBtn.setBounds(tabArea.removeFromLeft(tabArea.getWidth() / 2));
        fxTabBtn.setBounds(tabArea);

        // El panel activo ocupa TODO el resto del espacio superior
        if (currentTab == PickerTab) {
            pickerPanel.setBounds(area);
        } else {
            effectsPanel.setBounds(area);
        }
    }

private:
    PickerPanel& pickerPanel;
    EffectsPanel& effectsPanel;
    juce::TextButton pickerTabBtn, fxTabBtn;
    Tab currentTab = PickerTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftSidebar)
};