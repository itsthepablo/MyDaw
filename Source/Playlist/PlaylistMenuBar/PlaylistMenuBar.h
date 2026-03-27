#pragma once
#include <JuceHeader.h>

class PlaylistMenuBar : public juce::Component
{
public:
    PlaylistMenuBar();
    ~PlaylistMenuBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistMenuBar)
};