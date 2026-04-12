#include "CommandController.h"
#include "../../Engine/Core/AudioEngine.h"

CommandController::CommandController(DAWUIComponents& uiComponents, ViewManager& vManager, AudioEngine& engine)
    : ui(uiComponents), viewManager(vManager), audioEngine(engine)
{
    commandManager.registerAllCommandsForTarget(this);
}

juce::ApplicationCommandTarget* CommandController::getNextCommandTarget() {
    return nullptr; 
}

void CommandController::getAllCommands(juce::Array<juce::CommandID>& commands) {
    commands.add(DAWCommands::playStop);
    commands.add(DAWCommands::pause);
    commands.add(DAWCommands::toggleView);
}

void CommandController::getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& result) {
    if (id == DAWCommands::playStop) {
        result.setInfo("Play / Stop (Reset)", "Reproducir o detener volviendo al inicio", "Transport", 0);
        result.addDefaultKeypress(' ', 0);
    }
    else if (id == DAWCommands::pause) {
        result.setInfo("Pause / Resume", "Alternar pausa en la posición actual", "Transport", 0);
        result.addDefaultKeypress(' ', juce::ModifierKeys::ctrlModifier);
        result.addDefaultKeypress(' ', juce::ModifierKeys::commandModifier);
    }
    else if (id == DAWCommands::toggleView) {
        result.setInfo("Alternar Vista", "Interfaz", "UI", 0);
        result.addDefaultKeypress(juce::KeyPress::tabKey, 0);
    }
}

bool CommandController::perform(const juce::ApplicationCommandTarget::InvocationInfo& info) {
    if (info.commandID == DAWCommands::playStop) {
        if (ui.pianoRollUI.getIsPlaying()) {
            // --- STOP con RESET (Lógica original completa) ---
            ui.pianoRollUI.setPlaying(false);
            ui.playlistUI.isPlaying = false;
            
            ui.pianoRollUI.setPlayheadPos(0);
            ui.playlistUI.setPlayheadPos(0);
            
            // Notificar al motor para que resetee el reloj interno
            audioEngine.transportState.playheadPos.store(0.0f, std::memory_order_release);
            if (ui.playlistUI.onPlayheadSeekRequested) ui.playlistUI.onPlayheadSeekRequested(0.0f);

            ui.topMenuBar.playBtn.setButtonText("P");
            ui.topMenuBar.playBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            ui.topMenuBar.playBtn.setToggleState(false, juce::dontSendNotification);
        } else {
            // --- PLAY / RESUME ---
            ui.pianoRollUI.setPlaying(true);
            ui.playlistUI.isPlaying = true;
            
            ui.topMenuBar.playBtn.setButtonText("S");
            ui.topMenuBar.playBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
            ui.topMenuBar.playBtn.setToggleState(true, juce::dontSendNotification);
        }
        return true;
    }
    
    if (info.commandID == DAWCommands::pause) {
        if (ui.pianoRollUI.getIsPlaying()) {
            // --- PAUSE (Guardar posición exacta y detener) ---
            float currentPos = audioEngine.clock.currentPh;
            
            ui.pianoRollUI.setPlaying(false);
            ui.playlistUI.isPlaying = false;
            
            ui.pianoRollUI.setPlayheadPos(currentPos);
            ui.playlistUI.setPlayheadPos(currentPos);
            
            // CRÍTICO: Avisar al motor que mantenga esta posición mientras esté detenido
            audioEngine.transportState.playheadPos.store(currentPos, std::memory_order_release);

            ui.topMenuBar.playBtn.setButtonText("P");
            ui.topMenuBar.playBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            ui.topMenuBar.playBtn.setToggleState(false, juce::dontSendNotification);
        } else {
            // --- RESUME ---
            ui.pianoRollUI.setPlaying(true);
            ui.playlistUI.isPlaying = true;
            
            ui.topMenuBar.playBtn.setButtonText("S");
            ui.topMenuBar.playBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
            ui.topMenuBar.playBtn.setToggleState(true, juce::dontSendNotification);
        }
        return true;
    }

    if (info.commandID == DAWCommands::toggleView) {
        viewManager.toggleViewMode();
        return true;
    }

    return false;
}
