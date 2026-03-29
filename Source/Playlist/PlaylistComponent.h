#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <functional> 
#include <cmath>       
#include "../Tracks/TrackContainer.h" 
#include "Tools/PlaylistTool.h" 
#include "PlaylistMenuBar/PlaylistMenuBar.h" 
#include "ScrollBar/PlaylistNavigator.h" 
#include "ScrollBar/VerticalNavigator.h" 

struct TrackClip {
    Track* trackPtr;
    float startX;
    float width;
    juce::String name;
    AudioClipData* linkedAudio = nullptr;
    MidiClipData* linkedMidi = nullptr;
};

class PlaylistComponent : public juce::Component,
    public juce::FileDragAndDropTarget,
    public juce::DragAndDropTarget,
    public juce::KeyListener,
    private juce::Timer
{
public:
    PlaylistComponent();
    ~PlaylistComponent() override;

    // --- NUEVO: Antena receptora del Reloj Maestro ---
    std::function<float()> getPlaybackPosition;
    std::function<void(float)> onPlayheadSeekRequested;

    std::function<void(TrackType)> onNewTrackRequested;
    std::function<void(int)> onVerticalScroll;
    std::function<void(Track*, MidiClipData*)> onMidiClipDoubleClicked;
    std::function<void(MidiClipData*)> onMidiClipDeleted;
    std::function<void(MidiClipData*)> onPatternEdited;

    void notifyPatternEdited(MidiClipData* clip) {
        if (onPatternEdited && clip != nullptr) onPatternEdited(clip);
    }

    const juce::OwnedArray<Track>* tracksRef = nullptr;
    TrackContainer* trackContainer = nullptr;

    void setTrackContainer(TrackContainer* tc) {
        trackContainer = tc;
        if (tc) {
            setTracksReference(&(tc->getTracks()));
            tc->setTrackHeight(trackHeight);
        }
    }

    std::vector<TrackClip> clips;
    float hZoom = 1.0f;

    PlaylistMenuBar menuBar;
    PlaylistNavigator hNavigator;
    VerticalNavigator vBar;

    int draggingClipIndex = -1;
    int selectedClipIndex = -1;
    bool isResizingClip = false;
    float dragStartAbsX = 0.0f;
    float dragStartXOriginal = 0.0f;
    float dragStartWidth = 0.0f;

    int draggingNoteIndex = -1;
    bool isResizingNote = false;
    float dragStartNoteX = 0.0f;
    float dragStartNoteWidth = 0.0f;

    juce::CriticalSection* audioMutex = nullptr;

    const int menuBarH = 34;
    const int navigatorH = 51;
    const int timelineH = 35;
    const int vBarWidth = 32;

    float trackHeight = 100.0f;
    double snapPixels = 80.0;

    void setTracksReference(const juce::OwnedArray<Track>* tracks) {
        tracksRef = tracks;
        updateScrollBars();
        repaint();
    }

    void setExternalMutex(juce::CriticalSection* mutex) { audioMutex = mutex; }
    void addMidiClipToView(Track* targetTrack, MidiClipData* newClip);
    void updateScrollBars();

    void setTool(int toolId);

    float getPlayheadPos() const { return playheadAbsPos; }
    void setPlayheadPos(float newPos) { playheadAbsPos = newPos; repaint(); }

    float getLoopEndPos() const {
        double dynamicLoopEnd = 1280.0;
        for (const auto& clip : clips) {
            if (clip.startX + clip.width > dynamicLoopEnd) dynamicLoopEnd = clip.startX + clip.width;
        }
        return (float)(std::ceil((dynamicLoopEnd - 0.001) / 320.0) * 320.0);
    }

    double getTimelineLength() const {
        double defaultLength = 20.0 * 320.0; // 20 compases base (Estilo FL Studio)
        double maxTime = defaultLength;
        for (const auto& clip : clips) {
            double clipEnd = (double)clip.startX + (double)clip.width;
            if (clipEnd > maxTime) maxTime = clipEnd;
        }
        if (maxTime > defaultLength) maxTime += (2.0 * 320.0); // 2 compases de padding al final
        return maxTime;
    }

    void setBpm(double newBpm) { bpm = newBpm; repaint(); }
    double getBpm() const { return bpm; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void drawMinimap(juce::Graphics& g, juce::Rectangle<int> bounds);

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    void timerCallback() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    bool isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override;
    void itemDragEnter(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override;
    void itemDragExit(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override;
    void itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) override;

    int getTrackAtY(int y) const;
    int getClipAt(int x, int y) const;
    int getTrackY(Track* targetTrack) const;

    void deleteClip(int index);
    void deleteClipsByName(const juce::String& name, bool isMidi);
    void purgeClipsOfTrack(Track* track);

    float getAbsoluteXFromMouse(int mouseX) const {
        return (mouseX + (float)hNavigator.getCurrentRangeStart()) / hZoom;
    }

    bool isPlaying = false;

private:
    std::unique_ptr<PlaylistTool> activeTool;

    float playheadAbsPos = 0.0f;
    double bpm = 120.0;
    bool isExternalFileDragging = false;
    bool isInternalDragging = false;
    bool isDraggingTimeline = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistComponent)
};