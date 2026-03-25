#include "DAWCommandHandler.h"

DAWCommandHandler::DAWCommandHandler(CommandActions a) : actions(a) {}

juce::ApplicationCommandTarget* DAWCommandHandler::getNextCommandTarget() {
    return nullptr; // Aquí podrías devolver un plugin enfocado si quisieras
}

void DAWCommandHandler::getAllCommands(juce::Array<juce::CommandID>& commands) {
    commands.add(DAWCommands::playStop);
    commands.add(DAWCommands::toggleView);
}

void DAWCommandHandler::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& result) {
    if (id == DAWCommands::playStop) {
        result.setInfo("Play / Stop", "Control de transporte", "Transport", 0);
        result.addDefaultKeypress(' ', 0);
    }
    else if (id == DAWCommands::toggleView) {
        result.setInfo("Alternar Vista", "Interfaz", "UI", 0);
        result.addDefaultKeypress(juce::KeyPress::tabKey, 0);
    }
}

bool DAWCommandHandler::perform(const juce::ApplicationCommandTarget::InvocationInfo& info) {
    if (info.commandID == DAWCommands::playStop) {
        if (actions.onPlayStopRequested) actions.onPlayStopRequested();
        return true;
    }
    if (info.commandID == DAWCommands::toggleView) {
        if (actions.onToggleViewRequested) actions.onToggleViewRequested();
        return true;
    }
    return false;
}