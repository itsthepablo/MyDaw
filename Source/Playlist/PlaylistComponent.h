#pragma once
#include <JuceHeader.h>
#include "PlaylistClip.h"
#include <vector>
#include <memory>
#include <functional> 
#include <cmath>       
#include "../Tracks/UI/TrackContainer.h" 
#include "Tools/PlaylistTool.h" 
#include "PlaylistMenuBar/PlaylistMenuBar.h" 
#include "ScrollBar/PlaylistNavigator.h" 
#include "ScrollBar/VerticalNavigator.h" 
#include "../UI/Knobs/FloatingValueSlider.h"
#include "../NativePlugins/ChannelEQ/ChannelEQ_GUI.h"


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
    std::function<void(float)> onZoomChanged;

    std::function<void(TrackType)> onNewTrackRequested;
    std::function<void(int)> onVerticalScroll;
    std::function<void(Track*, MidiPattern*)> onMidiClipDoubleClicked;
    std::function<void(MidiPattern*)> onMidiClipDeleted;
    std::function<void(MidiPattern*)> onPatternEdited;
    std::function<void(Track*)> onClipSelected;

    void notifyPatternEdited(MidiPattern* clip) {
        if (onPatternEdited && clip != nullptr) onPatternEdited(clip);
    }

    const juce::OwnedArray<Track>* tracksRef = nullptr;
    TrackContainer* trackContainer = nullptr;
    Track* masterTrackPtr = nullptr;

    void setTrackContainer(TrackContainer* tc);
    void copyCallbacksFrom(const PlaylistComponent& other);

    std::vector<TrackClip> clips;
    float hZoom = 0.2f;

    PlaylistMenuBar menuBar;
    PlaylistNavigator hNavigator;
    VerticalNavigator vBar;

    int draggingClipIndex = -1;
    std::vector<int> selectedClipIndices;
    bool isResizingClip = false;
    float dragStartAbsX = 0.0f;
    float dragStartXOriginal = 0.0f;
    float dragStartWidth = 0.0f;
    bool isAltDragging = false;

    int draggingNoteIndex = -1;
    bool isResizingNote = false;
    float dragStartNoteX = 0.0f;
    float dragStartNoteWidth = 0.0f;

    struct DraggingAutoNode {
        AutomationClipData* clip = nullptr;
        int nodeIndex = -1;
        int tensionNodeIndex = -1;
    } draggingAutoNode;

    juce::CriticalSection* audioMutex = nullptr;
    AutomationClipData* hoveredAutoClip = nullptr;

    const int menuBarH = 34;
    const int navigatorH = 51;
    const int timelineH = 35;
    const int vBarWidth = 32;
    const int masterTrackH = 100;

    float trackHeight = 100.0f;
    double snapPixels = 80.0;

    void setTracksReference(const juce::OwnedArray<Track>* tracks) {
        tracksRef = tracks;
        updateScrollBars();
        repaint();
    }

    void setExternalMutex(juce::CriticalSection* mutex);
    void setMasterTrack(Track* mt);
    void addMidiClipToView(Track* targetTrack, MidiPattern* newClip);
    void addAudioClipToView(Track* targetTrack, AudioClip* newClip);
    void updateScrollBars();

    void setTool(int toolId);

    float getPlayheadPos() const { return playheadAbsPos; }
    void setPlayheadPos(float newPos) { playheadAbsPos = newPos; repaint(); }

    float getLoopEndPos() const {
        double dynamicLoopEnd = 1280.0;
        if (tracksRef) {
            for (auto* t : *tracksRef) {
                for (auto* ac : t->getAudioClips()) {
                    if (ac->getStartX() + ac->getWidth() > dynamicLoopEnd) dynamicLoopEnd = ac->getStartX() + ac->getWidth();
                }
                for (auto* mc : t->getMidiClips()) {
                    if (mc->getStartX() + mc->getWidth() > dynamicLoopEnd) dynamicLoopEnd = mc->getStartX() + mc->getWidth();
                }
            }
        }
        return (float)(std::ceil((dynamicLoopEnd - 0.001) / 320.0) * 320.0);
    }

    double getTimelineLength() const {
        double defaultLength = 20.0 * 320.0;
        double maxTime = defaultLength;
        if (tracksRef) {
            for (auto* t : *tracksRef) {
                for (auto* ac : t->getAudioClips()) {
                    double clipEnd = (double)ac->getStartX() + (double)ac->getWidth();
                    if (clipEnd > maxTime) maxTime = clipEnd;
                }
                for (auto* mc : t->getMidiClips()) {
                    double clipEnd = (double)mc->getStartX() + (double)mc->getWidth();
                    if (clipEnd > maxTime) maxTime = clipEnd;
                }
            }
        }
        if (maxTime > defaultLength) maxTime += (2.0 * 320.0);
        return maxTime;
    }

    double getMaxAudioDurationSecs() const {
        double maxPixels = 0.0;
        if (tracksRef) {
            for (auto* t : *tracksRef) {
                for (auto* ac : t->getAudioClips()) {
                    double clipEnd = (double)ac->getStartX() + (double)ac->getWidth();
                    if (clipEnd > maxPixels) maxPixels = clipEnd;
                }
                for (auto* mc : t->getMidiClips()) {
                    double clipEnd = (double)mc->getStartX() + (double)mc->getWidth();
                    if (clipEnd > maxPixels) maxPixels = clipEnd;
                }
            }
        }
        if (maxPixels == 0.0) return 1.0;
        return (maxPixels / 320.0) * 4.0 * (60.0 / bpm);
    }

    void setBpm(double newBpm) { bpm = newBpm; repaint(); }
    double getBpm() const { return bpm; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;
    void drawMinimap(juce::Graphics& g, juce::Rectangle<int> bounds);

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    
    void setSelectedTrack(Track* t);
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

    juce::MouseCursor getMouseCursor() override;

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

    void deleteSelectedClips();
    void deleteClipsByName(const juce::String& name, bool isMidi);
    void purgeClipsOfTrack(Track* track);

    float getAbsoluteXFromMouse(int mouseX) const {
        return (mouseX + (float)hNavigator.getCurrentRangeStart()) / hZoom;
    }

    bool isPlaying = false;

private:
    std::unique_ptr<PlaylistTool> activeTool;
    int currentToolId = 1;
    Track* currentVisualTrack = nullptr; // Guardia de recursión infinita

    float playheadAbsPos = 0.0f;
    double bpm = 120.0;
    bool isExternalFileDragging = false;
    bool isInternalDragging = false;
    bool isDraggingTimeline = false;

    // --- CONTROLES MASTER EN PLAYLIST ---
    FloatingValueSlider masterVolSlider;
    FloatingValueSlider masterPanSlider;
    juce::TextButton masterMuteBtn;
    juce::TextButton masterSoloBtn;
    juce::Label masterLabel;

    ChannelEQ_Editor eqEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistComponent)
};