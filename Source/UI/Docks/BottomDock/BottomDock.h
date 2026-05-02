#pragma once
#include <JuceHeader.h>
#include "../../../Theme/CustomTheme.h"
#include "../../Panels/ChannelRack/ChannelRackPanel.h"
#include "../../Panels/Instruments/InstrumentPanel.h"
#include "../../Panels/Inspector/InspectorPanel.h"
#include "../../Panels/Modulators/ModulatorRackPanel.h"
#include "../../Panels/VUMeter/VUMeterComponent.h"
#include "../../../Mixer/MixerComponent.h"

class BottomDock : public juce::Component {
public:
    enum Tab { RackTab, InstrumentTab, MixerTab, InspectorTab, ModulatorsTab };

    std::function<void()> onClose;
    std::function<void(Tab)> onTabChanged;

    BottomDock(ChannelRackPanel& rack, InstrumentPanel& inst, MixerComponent& mixer, InspectorPanel& inspector, VUMeterComponent& vu, ModulatorRackPanel& mod)
        : rackPanel(rack), instrumentPanel(inst), miniMixer(mixer), inspectorPanel(inspector), vuMeter(vu), modulatorsPanel(mod)
    {
        updateStyles();
        addAndMakeVisible(rackPanel);
        addAndMakeVisible(instrumentPanel);
        addAndMakeVisible(miniMixer);
        addAndMakeVisible(inspectorPanel);
        addAndMakeVisible(vuMeter);
        addAndMakeVisible(modulatorsPanel);

        addAndMakeVisible(rackTabBtn);
        rackTabBtn.setButtonText("CHANNEL RACK");
        rackTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        rackTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        rackTabBtn.setClickingTogglesState(true);
        rackTabBtn.setRadioGroupId(300);
        rackTabBtn.onClick = [this] { showTab(RackTab); };

        addAndMakeVisible(inspectorTabBtn);
        inspectorTabBtn.setButtonText("INSPECTOR");
        inspectorTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        inspectorTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        inspectorTabBtn.setClickingTogglesState(true);
        inspectorTabBtn.setRadioGroupId(300);
        inspectorTabBtn.onClick = [this] { showTab(InspectorTab); };

        addAndMakeVisible(instrumentTabBtn);
        instrumentTabBtn.setButtonText("INSTRUMENT");
        instrumentTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        instrumentTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        instrumentTabBtn.setClickingTogglesState(true);
        instrumentTabBtn.setRadioGroupId(300);
        instrumentTabBtn.onClick = [this] { showTab(InstrumentTab); };

        addAndMakeVisible(mixerTabBtn);
        mixerTabBtn.setButtonText("MIXER");
        mixerTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        mixerTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        mixerTabBtn.setClickingTogglesState(true);
        mixerTabBtn.setRadioGroupId(300);
        mixerTabBtn.onClick = [this] { showTab(MixerTab); };

        addAndMakeVisible(modulatorsTabBtn);
        modulatorsTabBtn.setButtonText("MODULATORS");
        modulatorsTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        modulatorsTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        modulatorsTabBtn.setClickingTogglesState(true);
        modulatorsTabBtn.setRadioGroupId(300);
        modulatorsTabBtn.onClick = [this] { showTab(ModulatorsTab); };

        addAndMakeVisible(closeBtn);
        closeBtn.setButtonText("X");
        closeBtn.setTooltip("Cerrar Panel Inferior");
        closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        closeBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
        closeBtn.onClick = [this] { if (onClose) onClose(); };

        showTab(RackTab);
    }

    void updateStyles() {
        if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
            auto bg = theme->getSkinColor("DOCK_BG", juce::Colour(25, 27, 30));
            rackTabBtn.setColour(juce::TextButton::buttonColourId, bg);
            inspectorTabBtn.setColour(juce::TextButton::buttonColourId, bg);
            instrumentTabBtn.setColour(juce::TextButton::buttonColourId, bg);
            mixerTabBtn.setColour(juce::TextButton::buttonColourId, bg);
            modulatorsTabBtn.setColour(juce::TextButton::buttonColourId, bg);
            closeBtn.setColour(juce::TextButton::buttonColourId, bg);
        }
    }

    void lookAndFeelChanged() override {
        updateStyles();
        repaint();
    }

    void showTab(Tab tab) {
        currentTab = tab;
        rackPanel.setVisible(tab == RackTab);
        instrumentPanel.setVisible(tab == InstrumentTab);
        miniMixer.setVisible(tab == MixerTab);
        inspectorPanel.setVisible(tab == InspectorTab);
        vuMeter.setVisible(tab == InspectorTab);
        modulatorsPanel.setVisible(tab == ModulatorsTab);

        if (tab == RackTab) rackTabBtn.setToggleState(true, juce::dontSendNotification);
        else if (tab == InspectorTab) inspectorTabBtn.setToggleState(true, juce::dontSendNotification);
        else if (tab == MixerTab) mixerTabBtn.setToggleState(true, juce::dontSendNotification);
        else if (tab == ModulatorsTab) modulatorsTabBtn.setToggleState(true, juce::dontSendNotification);
        else instrumentTabBtn.setToggleState(true, juce::dontSendNotification);

        if (onTabChanged) onTabChanged(tab);

        resized();
    }

    Tab getCurrentTab() const { return currentTab; }

    void resized() override {
        auto area = getLocalBounds();
        auto tabArea = area.removeFromTop(25);

        closeBtn.setBounds(tabArea.removeFromRight(30).reduced(2));

        rackTabBtn.setBounds(tabArea.removeFromLeft(120));
        inspectorTabBtn.setBounds(tabArea.removeFromLeft(120));
        instrumentTabBtn.setBounds(tabArea.removeFromLeft(120));
        mixerTabBtn.setBounds(tabArea.removeFromLeft(100));
        modulatorsTabBtn.setBounds(tabArea.removeFromLeft(120));

        if (currentTab == RackTab) rackPanel.setBounds(area);
        else if (currentTab == InspectorTab) 
        {
            inspectorPanel.setBounds(area.removeFromLeft(area.getWidth() / 2));
            vuMeter.setBounds(area.removeFromLeft(area.getWidth() * 0.25f));
        }
        else if (currentTab == MixerTab) miniMixer.setBounds(area);
        else if (currentTab == ModulatorsTab) modulatorsPanel.setBounds(area);
        else instrumentPanel.setBounds(area);
    }

    void paint(juce::Graphics& g) override {
        if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
            g.setColour(theme->getSkinColor("DOCK_BG", juce::Colour(20, 22, 25)));
        } else {
            g.setColour(juce::Colour(20, 22, 25));
        }
        g.fillRect(0, 0, getWidth(), 25);
    }

private:
    ChannelRackPanel& rackPanel;
    InstrumentPanel& instrumentPanel;
    MixerComponent& miniMixer;
    InspectorPanel& inspectorPanel;
    VUMeterComponent& vuMeter;
    ModulatorRackPanel& modulatorsPanel;
    juce::TextButton rackTabBtn, inspectorTabBtn, instrumentTabBtn, mixerTabBtn, modulatorsTabBtn, closeBtn;
    Tab currentTab = RackTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomDock)
};