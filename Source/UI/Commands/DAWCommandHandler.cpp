#include "DAWCommandHandler.h"

DAWCommandHandler::DAWCommandHandler(CommandActions a) : actions(a) {}

juce::ApplicationCommandTarget* DAWCommandHandler::getNextCommandTarget() {
    return nullptr; 
}

void DAWCommandHandler::getAllCommands(juce::Array<juce::CommandID>& commands) {
    commands.add(DAWCommands::playStop);
    commands.add(DAWCommands::toggleView);
}

void DAWCommandHandler::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& result) {
    if (id == DAWCommands::playStop) {
        result.setInfo("Play / Stop", "Control de transporte", "Transport", 0);
        
        // Espacio Normal
        result.addDefaultKeypress(' ', 0);
        
        // Ctrl + Espacio y Cmd + Espacio (Para la pausa, el Puente lo detectará en tiempo real)
        result.addDefaultKeypress(' ', juce::ModifierKeys::ctrlModifier);
        result.addDefaultKeypress(' ', juce::ModifierKeys::commandModifier);
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