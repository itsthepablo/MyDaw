#pragma once
#include <JuceHeader.h>

class ToolbarButtons : public juce::Component {
public:
    juce::TextButton pointerBtn{ "Sel" };
    juce::TextButton drawBtn{ "Lap" };
    juce::TextButton cutBtn{ "Cor" };
    juce::TextButton eraseBtn{ "Bor" };

    juce::ComboBox snapCombo;

    std::function<void(int)> onToolChanged;
    std::function<void(double)> onSnapChanged;

    ToolbarButtons() {
        addAndMakeVisible(pointerBtn);
        addAndMakeVisible(drawBtn);
        addAndMakeVisible(cutBtn);
        addAndMakeVisible(eraseBtn);

        pointerBtn.setClickingTogglesState(true);
        drawBtn.setClickingTogglesState(true);
        cutBtn.setClickingTogglesState(true);
        eraseBtn.setClickingTogglesState(true);

        pointerBtn.setRadioGroupId(101);
        drawBtn.setRadioGroupId(101);
        cutBtn.setRadioGroupId(101);
        eraseBtn.setRadioGroupId(101);

        pointerBtn.setToggleState(true, juce::dontSendNotification);

        auto toolClick = [this](int id) { if (onToolChanged) onToolChanged(id); };
        pointerBtn.onClick = [this, toolClick] { toolClick(1); };
        drawBtn.onClick = [this, toolClick] { toolClick(2); };
        cutBtn.onClick = [this, toolClick] { toolClick(3); };
        eraseBtn.onClick = [this, toolClick] { toolClick(4); };

        addAndMakeVisible(snapCombo);
        snapCombo.addItem("Libre (None)", 1);
        snapCombo.addItem("1/8 Beat (Fusa)", 2);
        snapCombo.addItem("1/4 Beat (Step)", 3);
        snapCombo.addItem("1/2 Beat (Corchea)", 4);
        snapCombo.addItem("1 Beat (Negra)", 5);
        snapCombo.addItem("1 Bar (Compas)", 6);

        snapCombo.setSelectedId(5, juce::dontSendNotification);
        snapCombo.onChange = [this] {
            if (!onSnapChanged) return;
            switch (snapCombo.getSelectedId()) {
            case 1: onSnapChanged(1.0); break;
            case 2: onSnapChanged(10.0); break;
            case 3: onSnapChanged(20.0); break;
            case 4: onSnapChanged(40.0); break;
            case 5: onSnapChanged(80.0); break;
            case 6: onSnapChanged(320.0); break;
            }
            };
    }

    void resized() override {
        auto area = getLocalBounds().reduced(2);

        snapCombo.setBounds(area.removeFromLeft(120).reduced(0, 4));
        area.removeFromLeft(5);

        int btnW = 35;
        pointerBtn.setBounds(area.removeFromLeft(btnW).reduced(2));
        drawBtn.setBounds(area.removeFromLeft(btnW).reduced(2));
        cutBtn.setBounds(area.removeFromLeft(btnW).reduced(2));
        eraseBtn.setBounds(area.removeFromLeft(btnW).reduced(2));
    }
};