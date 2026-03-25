#pragma once
#include <JuceHeader.h>
#include "CommandIDs.h"

struct CommandActions {
    std::function<void()> onPlayStopRequested;
    std::function<void()> onToggleViewRequested;
};

class DAWCommandHandler : public juce::ApplicationCommandTarget {
public:
    DAWCommandHandler(CommandActions actions);

    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) override;

private:
    CommandActions actions;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DAWCommandHandler)
};