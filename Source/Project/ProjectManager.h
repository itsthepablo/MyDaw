#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Playlist/PlaylistComponent.h"
#include "../UI/PickerPanel.h"
#include "../Engine/Core/AudioEngine.h"

class ProjectManager {
public:
    static void saveProject(TrackContainer& trackContainer, AudioEngine& engine, std::unique_ptr<juce::FileChooser>& fileChooser);
    
    static void loadProject(const juce::File& file, TrackContainer& trackContainer, AudioEngine& engine, juce::CriticalSection* audioMutex, PlaylistComponent& playlistUI, EffectsPanel& effectsUI, PickerPanel& pickerUI, std::function<void()> onFinished);
};