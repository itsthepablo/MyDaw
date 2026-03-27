#pragma once
#include <JuceHeader.h>

class PlaylistComponent;
class Track;

class PlaylistActionHandler {
public:
    static void deleteClip(PlaylistComponent& p, int index);
    static void deleteClipsByName(PlaylistComponent& p, const juce::String& name, bool isMidi);
    static void purgeClipsOfTrack(PlaylistComponent& p, Track* track);
    static void handleDoubleClick(PlaylistComponent& p, const juce::MouseEvent& e);
};