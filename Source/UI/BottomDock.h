#pragma once
#include <JuceHeader.h>
#include "ChannelRackPanel.h"
#include "../Effects/EffectsPanel.h"
#include "../Instruments/InstrumentPanel.h"

class BottomDock : public juce::Component {
public:
    enum Tab { RackTab, EffectsTab, InstrumentTab };

    std::function<void()> onClose;

    BottomDock(ChannelRackPanel& rack, EffectsPanel& fx, InstrumentPanel& inst)
        : rackPanel(rack), effectsPanel(fx), instrumentPanel(inst)
    {
        addAndMakeVisible(rackPanel);
        addAndMakeVisible(effectsPanel);
        addAndMakeVisible(instrumentPanel);

        addAndMakeVisible(rackTabBtn);
        rackTabBtn.setButtonText("CHANNEL RACK");
        rackTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        rackTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        rackTabBtn.setClickingTogglesState(true);
        rackTabBtn.setRadioGroupId(300);
        rackTabBtn.onClick = [this] { showTab(RackTab); };

        addAndMakeVisible(effectsTabBtn);
        effectsTabBtn.setButtonText("EFFECTS / DEVICE");
        effectsTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        effectsTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        effectsTabBtn.setClickingTogglesState(true);
        effectsTabBtn.setRadioGroupId(300);
        effectsTabBtn.onClick = [this] { showTab(EffectsTab); };

        addAndMakeVisible(instrumentTabBtn);
        instrumentTabBtn.setButtonText("INSTRUMENT");
        instrumentTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        instrumentTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        instrumentTabBtn.setClickingTogglesState(true);
        instrumentTabBtn.setRadioGroupId(300);
        instrumentTabBtn.onClick = [this] { showTab(InstrumentTab); };

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
        rackPanel.setVisible(tab == RackTab);
        effectsPanel.setVisible(tab == EffectsTab);
        instrumentPanel.setVisible(tab == InstrumentTab);

        if (tab == RackTab) rackTabBtn.setToggleState(true, juce::dontSendNotification);
        else if (tab == EffectsTab) effectsTabBtn.setToggleState(true, juce::dontSendNotification);
        else instrumentTabBtn.setToggleState(true, juce::dontSendNotification);

        resized();
    }

    Tab getCurrentTab() const { return currentTab; }

    void resized() override {
        auto area = getLocalBounds();
        auto tabArea = area.removeFromTop(25);

        closeBtn.setBounds(tabArea.removeFromRight(30).reduced(2));

        rackTabBtn.setBounds(tabArea.removeFromLeft(120));
        effectsTabBtn.setBounds(tabArea.removeFromLeft(140));
        instrumentTabBtn.setBounds(tabArea.removeFromLeft(120));

        if (currentTab == RackTab) rackPanel.setBounds(area);
        else if (currentTab == EffectsTab) effectsPanel.setBounds(area);
        else instrumentPanel.setBounds(area);
    }

    void paint(juce::Graphics& g) override {
        g.setColour(juce::Colour(20, 22, 25));
        g.fillRect(0, 0, getWidth(), 25);
    }

private:
    ChannelRackPanel& rackPanel;
    EffectsPanel& effectsPanel;
    InstrumentPanel& instrumentPanel;
    juce::TextButton rackTabBtn, effectsTabBtn, instrumentTabBtn, closeBtn;
    Tab currentTab = EffectsTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomDock)
};