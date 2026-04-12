#pragma once
#include <JuceHeader.h>
#include "../UI/UIManager.h"
#include "../Engine/Core/AudioEngine.h"

class ProjectController {
public:
    ProjectController(DAWUIComponents& uiComponents, AudioEngine& engine, juce::CriticalSection& mutex);

    void saveProject();
    void loadProject(const juce::File& file, std::function<void()> onFinished);

private:
    DAWUIComponents& ui;
    AudioEngine& audioEngine;
    juce::CriticalSection& audioMutex;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectController)
};
