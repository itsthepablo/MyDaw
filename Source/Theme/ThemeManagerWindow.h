#pragma once
#include <JuceHeader.h>
#include "ThemeManagerPanel.h"

class ThemeManagerWindow : public juce::DocumentWindow
{
public:
    ThemeManagerWindow(const juce::String& name)
        : DocumentWindow(name, 
                         juce::Desktop::getInstance().getDefaultLookAndFeel()
                         .findColour(juce::ResizableWindow::backgroundColourId),
                         allButtons)
    {
        setUsingNativeTitleBar(true);
        setContentOwned(new ThemeManagerPanel(), true);
        setResizable(true, false);
        setResizeLimits(300, 400, 600, 1000);
        centreWithSize(400, 600);
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        setVisible(false);
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ThemeManagerWindow)
};
