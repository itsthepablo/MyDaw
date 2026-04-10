#pragma once
#include <JuceHeader.h>
#include "../Tracks/UI/TrackContainer.h"
#include "../Playlist/PlaylistComponent.h"
#include "../Engine/Core/AudioEngine.h"
#include "../UI/Panels/Effects/EffectsPanel.h"
#include "../UI/Panels/Browsers/PickerPanel.h"

class ProjectManager
{
public:
    // Guarda el proyecto completo (tracks, clips, plugins, sends) a un archivo .perritogordo
    static void saveProject(TrackContainer& trackContainer,
                            AudioEngine& engine,
                            std::unique_ptr<juce::FileChooser>& fileChooser);

    // Carga un proyecto desde un archivo .perritogordo y reconstruye el estado completo
    static void loadProject(const juce::File& file,
                            TrackContainer& trackContainer,
                            AudioEngine& engine,
                            juce::CriticalSection* audioMutex,
                            PlaylistComponent& playlistUI,
                            EffectsPanel& effectsUI,
                            PickerPanel& pickerUI,
                            std::function<void()> onFinished);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectManager)
};