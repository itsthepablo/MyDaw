#pragma once
#include <JuceHeader.h>
#include <functional>

class PlaylistMenuBar : public juce::Component
{
public:
    PlaylistMenuBar();
    ~PlaylistMenuBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::TextButton btnUndo{ "Undo" };
    juce::TextButton btnRedo{ "Redo" };
    
    juce::TextButton btnPointer{ "Ptr" };
    juce::TextButton btnScissor{ "Cut" };
    juce::TextButton btnEraser{ "Erase" };
    juce::TextButton btnMute{ "Mute" };

    juce::ComboBox snapBox{ "Snap" };

    std::function<void()> onUndo;
    std::function<void()> onRedo;
    std::function<void(int)> onToolChanged;
    std::function<void(double)> onSnapChanged;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistMenuBar)
};