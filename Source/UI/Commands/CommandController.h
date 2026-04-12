#pragma once
#include <JuceHeader.h>
#include "CommandIDs.h"
#include "../UIManager.h"
#include "../Layout/ViewManager.h"

class AudioEngine;

class CommandController : public juce::ApplicationCommandTarget {
public:
    CommandController(DAWUIComponents& uiComponents, ViewManager& vManager, AudioEngine& engine);
    ~CommandController() override = default;

    juce::ApplicationCommandManager& getCommandManager() { return commandManager; }

    // Implementación de ApplicationCommandTarget
    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) override;

private:
    juce::ApplicationCommandManager commandManager;
    
    DAWUIComponents& ui;
    ViewManager& viewManager;
    AudioEngine& audioEngine;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CommandController)
};
