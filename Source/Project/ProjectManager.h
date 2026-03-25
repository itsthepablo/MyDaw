#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Playlist/PlaylistComponent.h"
#include "../UI/PickerPanel.h"

class ProjectManager {
public:
    static void saveProject(TrackContainer& trackContainer, std::unique_ptr<juce::FileChooser>& fileChooser);
    
    static void loadProject(const juce::File& file, 
                           TrackContainer& trackContainer, 
                           juce::CriticalSection& audioMutex,
                           PlaylistComponent& playlistUI,
                           PickerPanel& pickerPanelUI,
                           std::function<void()> onFinished);
};