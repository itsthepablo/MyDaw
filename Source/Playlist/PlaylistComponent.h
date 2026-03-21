#pragma once
#include <JuceHeader.h>
#include <vector>
#include "../Tracks/Track.h" 

struct TrackClip {
    Track* trackPtr;
    float startX;
    float width;
    juce::String name;
    AudioClipData* linkedAudio = nullptr;
};

class PlaylistComponent : public juce::Component,
    public juce::FileDragAndDropTarget,
    private juce::ScrollBar::Listener,
    private juce::Timer
{
public:
    PlaylistComponent();
    ~PlaylistComponent() override;

    std::function<void()> onNewTrackRequested;
    std::function<void(int)> onVerticalScroll;

    void setTracksReference(const juce::OwnedArray<Track>* tracks) {
        tracksRef = tracks;
        updateScrollBars();
        repaint();
    }

    void updateScrollBars();

    float getPlayheadPos() const { return playheadAbsPos; }
    void setPlayheadPos(float newPos) { playheadAbsPos = newPos; repaint(); }

    // NUEVO: Calcula el final del loop basándose en el audio más lejano (Mínimo 4 compases)
    float getLoopEndPos() const {
        double dynamicLoopEnd = 1280.0; // Mínimo de 4 compases por defecto (4 * 320px)
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
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;
    void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
    void timerCallback() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void fileDragEnter(const juce::StringArray& files, int x, int y) override;
    void fileDragExit(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    bool isPlaying = false;

private:
    const juce::OwnedArray<Track>* tracksRef = nullptr;
    std::vector<TrackClip> clips;
    float playheadAbsPos = 0.0f;
    double bpm = 120.0;

    const int timelineH = 35;
    const int scrollBarSize = 16;
    const float trackHeight = 100.0f;

    juce::ScrollBar hBar{ false };
    juce::ScrollBar vBar{ true };
    float hZoom = 1.0f;

    int draggingClipIndex = -1;
    bool isResizingClip = false;
    float dragStartAbsX = 0;
    float dragStartXOriginal = 0; // CORREGIDO: Nombre restaurado
    float dragStartWidth = 0;

    bool isExternalFileDragging = false;

    double getSnapPixels() const { return 80.0; }
    int getTrackAtY(int y) const;
    int getClipAt(int x, int y) const;
    int getTrackY(Track* targetTrack) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistComponent)
};