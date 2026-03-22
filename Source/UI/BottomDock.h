#pragma once
#include <JuceHeader.h>
#include "../Mixer/MixerComponent.h"
#include "ChannelRackPanel.h"

class BottomDock : public juce::Component {
public:
    enum Tab { MixerTab, RackTab };

    std::function<void()> onClose; // Callback para avisar que queremos cerrar el panel

    BottomDock(MixerComponent& mixer, ChannelRackPanel& rack)
        : mixerPanel(mixer), rackPanel(rack)
    {
        addAndMakeVisible(mixerPanel);
        addAndMakeVisible(rackPanel);

        // --- PESTAÑA DEL MIXER ---
        addAndMakeVisible(mixerTabBtn);
        mixerTabBtn.setButtonText("MIXER");
        mixerTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        mixerTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        mixerTabBtn.setClickingTogglesState(true);
        mixerTabBtn.setRadioGroupId(300);
        mixerTabBtn.onClick = [this] { showTab(MixerTab); };

        // --- PESTAÑA DEL RACK ---
        addAndMakeVisible(rackTabBtn);
        rackTabBtn.setButtonText("CHANNEL RACK");
        rackTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        rackTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        rackTabBtn.setClickingTogglesState(true);
        rackTabBtn.setRadioGroupId(300);
        rackTabBtn.onClick = [this] { showTab(RackTab); };

        // --- BOTÓN CERRAR [X] ---
        addAndMakeVisible(closeBtn);
        closeBtn.setButtonText("X");
        closeBtn.setTooltip("Cerrar Panel Inferior");
        closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        closeBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
        closeBtn.onClick = [this] { if (onClose) onClose(); };

        showTab(RackTab);
    }

    void showTab(Tab tab) {
        currentTab = tab;
        if (tab == MixerTab) {
            mixerTabBtn.setToggleState(true, juce::dontSendNotification);
            mixerPanel.setVisible(true);
            rackPanel.setVisible(false);
        }
        else {
            rackTabBtn.setToggleState(true, juce::dontSendNotification);
            mixerPanel.setVisible(false);
            rackPanel.setVisible(true);
        }
        resized();
    }

    Tab getCurrentTab() const { return currentTab; }

    void resized() override {
        auto area = getLocalBounds();

        auto tabArea = area.removeFromTop(25);

        // Posicionamos la [X] a la derecha
        closeBtn.setBounds(tabArea.removeFromRight(30).reduced(2));

        // Pestañas a la izquierda
        mixerTabBtn.setBounds(tabArea.removeFromLeft(100));
        rackTabBtn.setBounds(tabArea.removeFromLeft(120));

        juce::Rectangle<int> lineArea(0, 25, getWidth(), 2);

        if (currentTab == MixerTab) mixerPanel.setBounds(area);
        else rackPanel.setBounds(area);
    }

    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colour(20, 22, 25));
        g.fillRect(0, 0, getWidth(), 25);
    }

private:
    MixerComponent& mixerPanel;
    ChannelRackPanel& rackPanel;
    juce::TextButton mixerTabBtn, rackTabBtn, closeBtn; // Añadido closeBtn
    Tab currentTab = RackTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomDock)
};