#pragma once
#include <JuceHeader.h>
#include "../Mixer/MixerComponent.h"
#include "ChannelRackPanel.h"
#include "../Effects/EffectsPanel.h" // <-- AÑADIDO

class BottomDock : public juce::Component {
public:
    enum Tab { MixerTab, RackTab, EffectsTab }; // <-- AÑADIDO Efectos

    std::function<void()> onClose;

    BottomDock(MixerComponent& mixer, ChannelRackPanel& rack, EffectsPanel& fx)
        : mixerPanel(mixer), rackPanel(rack), effectsPanel(fx)
    {
        addAndMakeVisible(mixerPanel);
        addAndMakeVisible(rackPanel);
        addAndMakeVisible(effectsPanel);

        addAndMakeVisible(mixerTabBtn);
        mixerTabBtn.setButtonText("MIXER");
        mixerTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        mixerTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        mixerTabBtn.setClickingTogglesState(true);
        mixerTabBtn.setRadioGroupId(300);
        mixerTabBtn.onClick = [this] { showTab(MixerTab); };

        addAndMakeVisible(rackTabBtn);
        rackTabBtn.setButtonText("CHANNEL RACK");
        rackTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        rackTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        rackTabBtn.setClickingTogglesState(true);
        rackTabBtn.setRadioGroupId(300);
        rackTabBtn.onClick = [this] { showTab(RackTab); };

        // --- PESTAÑA EFFECTS ---
        addAndMakeVisible(effectsTabBtn);
        effectsTabBtn.setButtonText("EFFECTS / DEVICE");
        effectsTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        effectsTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        effectsTabBtn.setClickingTogglesState(true);
        effectsTabBtn.setRadioGroupId(300);
        effectsTabBtn.onClick = [this] { showTab(EffectsTab); };

        addAndMakeVisible(closeBtn);
        closeBtn.setButtonText("X");
        closeBtn.setTooltip("Cerrar Panel Inferior");
        closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        closeBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
        closeBtn.onClick = [this] { if (onClose) onClose(); };

        showTab(EffectsTab);
    }

    void showTab(Tab tab) {
        currentTab = tab;
        mixerPanel.setVisible(tab == MixerTab);
        rackPanel.setVisible(tab == RackTab);
        effectsPanel.setVisible(tab == EffectsTab);

        if (tab == MixerTab) mixerTabBtn.setToggleState(true, juce::dontSendNotification);
        else if (tab == RackTab) rackTabBtn.setToggleState(true, juce::dontSendNotification);
        else effectsTabBtn.setToggleState(true, juce::dontSendNotification);

        resized();
    }

    Tab getCurrentTab() const { return currentTab; }

    void resized() override {
        auto area = getLocalBounds();
        auto tabArea = area.removeFromTop(25);

        closeBtn.setBounds(tabArea.removeFromRight(30).reduced(2));

        mixerTabBtn.setBounds(tabArea.removeFromLeft(100));
        rackTabBtn.setBounds(tabArea.removeFromLeft(120));
        effectsTabBtn.setBounds(tabArea.removeFromLeft(140));

        if (currentTab == MixerTab) mixerPanel.setBounds(area);
        else if (currentTab == RackTab) rackPanel.setBounds(area);
        else effectsPanel.setBounds(area);
    }

    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colour(20, 22, 25));
        g.fillRect(0, 0, getWidth(), 25);
    }

private:
    MixerComponent& mixerPanel;
    ChannelRackPanel& rackPanel;
    EffectsPanel& effectsPanel;
    juce::TextButton mixerTabBtn, rackTabBtn, effectsTabBtn, closeBtn;
    Tab currentTab = EffectsTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomDock)
};