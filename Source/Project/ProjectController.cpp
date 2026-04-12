#include "ProjectController.h"
#include "ProjectManager.h"

ProjectController::ProjectController(DAWUIComponents& uiComponents, AudioEngine& engine, juce::CriticalSection& mutex)
    : ui(uiComponents), audioEngine(engine), audioMutex(mutex)
{
}

void ProjectController::saveProject() {
    ProjectManager::saveProject(ui.trackContainer, audioEngine, fileChooser);
}

void ProjectController::loadProject(const juce::File& file, std::function<void()> onFinished) {
    ProjectManager::loadProject(file, ui.trackContainer, audioEngine, &audioMutex, 
                                ui.playlistUI, ui.effectsPanelUI, ui.pickerPanelUI, 
                                onFinished);
}
