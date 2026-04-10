#pragma once
#include <JuceHeader.h>
#include "../../Panels/Browsers/PickerPanel.h"
#include "../../Panels/Browsers/FileBrowserPanel.h" 
#include "../../../Theme/CustomTheme.h"

class LeftSidebar : public juce::Component {
public:
    enum Tab { PickerTab, FilesTab };

    std::function<void()> onClose;

    LeftSidebar(PickerPanel& picker, FileBrowserPanel& files)
        : pickerPanel(picker), filesPanel(files)
    {
        updateStyles();
        addAndMakeVisible(pickerPanel);
        addAndMakeVisible(filesPanel);

        addAndMakeVisible(pickerTabBtn);
        pickerTabBtn.setButtonText("PICKER");
        pickerTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        pickerTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        pickerTabBtn.setClickingTogglesState(true);
        pickerTabBtn.setRadioGroupId(200);
        pickerTabBtn.onClick = [this] { showTab(PickerTab); };

        addAndMakeVisible(filesTabBtn);
        filesTabBtn.setButtonText("FILES");
        filesTabBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        filesTabBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(45, 48, 52));
        filesTabBtn.setClickingTogglesState(true);
        filesTabBtn.setRadioGroupId(200);
        filesTabBtn.onClick = [this] { showTab(FilesTab); };

        addAndMakeVisible(closeBtn);
        closeBtn.setButtonText("X");
        closeBtn.setTooltip("Cerrar Panel Lateral");
        closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(25, 27, 30));
        closeBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
        closeBtn.onClick = [this] { if (onClose) onClose(); };

        showTab(FilesTab);
    }

    void updateStyles() {
        if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
            auto bg = theme->getSkinColor("SIDEBAR_BG", juce::Colour(25, 27, 30));
            pickerTabBtn.setColour(juce::TextButton::buttonColourId, bg);
            filesTabBtn.setColour(juce::TextButton::buttonColourId, bg);
            closeBtn.setColour(juce::TextButton::buttonColourId, bg);
        }
    }

    void lookAndFeelChanged() override {
        updateStyles();
        repaint();
    }

    void showTab(Tab tab) {
        currentTab = tab;

        pickerPanel.setVisible(tab == PickerTab);
        filesPanel.setVisible(tab == FilesTab);

        if (tab == PickerTab) pickerTabBtn.setToggleState(true, juce::dontSendNotification);
        else filesTabBtn.setToggleState(true, juce::dontSendNotification);

        resized();
    }

    Tab getCurrentTab() const { return currentTab; }

    void resized() override {
        auto area = getLocalBounds();
        auto tabArea = area.removeFromBottom(30);

        closeBtn.setBounds(tabArea.removeFromRight(30).reduced(2));

        int tabW = tabArea.getWidth() / 2;
        pickerTabBtn.setBounds(tabArea.removeFromLeft(tabW));
        filesTabBtn.setBounds(tabArea);

        if (currentTab == PickerTab) pickerPanel.setBounds(area);
        else filesPanel.setBounds(area);
    }

private:
    PickerPanel& pickerPanel;
    FileBrowserPanel& filesPanel;

    juce::TextButton pickerTabBtn, filesTabBtn, closeBtn;
    Tab currentTab = FilesTab;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LeftSidebar)
};