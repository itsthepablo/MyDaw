#pragma once
#include <JuceHeader.h>
#include "../UI/UIManager.h"
#include "../Engine/Core/AudioEngine.h"

class SelectionManager {
public:
    SelectionManager(DAWUIComponents& uiComponents, AudioEngine& engine);

    void selectTrack(Track* t, bool isMaster);

private:
    DAWUIComponents& ui;
    AudioEngine& audioEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelectionManager)
};
