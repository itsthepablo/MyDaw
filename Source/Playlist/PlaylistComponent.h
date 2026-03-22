#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>
#include <functional> 
#include <cmath>       
#include "../Tracks/Track.h" 
#include "Tools/PlaylistTool.h" 

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
    public juce::KeyListener, 
    private juce::ScrollBar::Listener,
    private juce::Timer
{
public:
    PlaylistComponent();
    ~PlaylistComponent() override;

    std::function<void()> onNewTrackRequested;
    std::function<void(int)> onVerticalScroll;
    
    std::function<void(Track*, MidiClipData*)> onMidiClipDoubleClicked;
    std::function<void(MidiClipData*)> onMidiClipDeleted; 

    // --- VARIABLES PÚBLICAS ---
    const juce::OwnedArray<Track>* tracksRef = nullptr;
    std::vector<TrackClip> clips;
    float hZoom = 1.0f;
    juce::ScrollBar hBar{ false };
    juce::ScrollBar vBar{ true };
    
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
    const int timelineH = 35;
    const float trackHeight = 100.0f;
    const int scrollBarSize = 16; 
    
    // NUEVO: Variable global de cuantización (Defecto 1 Beat)
    double snapPixels = 80.0; 

    void setTracksReference(const juce::OwnedArray<Track>* tracks) {
        tracksRef = tracks;
        updateScrollBars();
        repaint();
    }

    void setExternalMutex(juce::CriticalSection* mutex) { audioMutex = mutex; }
    void addMidiClipToView(Track* targetTrack, MidiClipData* newClip);
    void updateScrollBars();

    // NUEVO: Inyector de Herramientas
    void setTool(int toolId);

    float getPlayheadPos() const { return playheadAbsPos; }
    void setPlayheadPos(float newPos) { playheadAbsPos = newPos; repaint(); }

    float getLoopEndPos() const {
        double dynamicLoopEnd = 1280.0; 
        for (const auto& clip : clips) {
            if (clip.startX + clip.width > dynamicLoopEnd) {
                dynamicLoopEnd = clip.startX + clip.width;
            }
        }
        return (float)(std::ceil((dynamicLoopEnd - 0.001) / 320.0) * 320.0);
    }

    void setBpm(double newBpm) { bpm = newBpm; repaint(); }
    double getBpm() const { return bpm; }

    void paint(juce::Graphics& g) override;
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override; 
    
    void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
    void timerCallback() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    int getTrackAtY(int y) const;
    int getClipAt(int x, int y) const;
    int getTrackY(Track* targetTrack) const;
    void deleteClip(int index); 

    bool isPlaying = false;

private:
    std::unique_ptr<PlaylistTool> activeTool; 

    float playheadAbsPos = 0.0f;
    double bpm = 120.0;
    bool isExternalFileDragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistComponent)
};