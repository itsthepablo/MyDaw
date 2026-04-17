#pragma once
#include <JuceHeader.h>
#include <vector>
#include <set>
#include <functional>
#include "../Data/GlobalData.h" 
#include "../Data/Track.h" 
#include "../Playlist/ScrollBar/PlaylistNavigator.h" 
#include "../Playlist/ScrollBar/VerticalNavigator.h" 

enum class ToolMode { Draw, Select };

struct FadingEntity {
    int pitch;
    int x;
    int width;
    float alpha;
    bool isDeleting;
};

class PianoRollComponent : public juce::Component,
    public juce::KeyListener,
    public juce::FileDragAndDropTarget,
    private juce::Timer
{
public:
    PianoRollComponent();
    ~PianoRollComponent() override;

    std::function<float()> getPlaybackPosition;

    std::function<void(MidiPattern*)> onPatternEdited;
    void notifyPatternEdited() {
        if (onPatternEdited && activeClip) onPatternEdited(activeClip);
    }

    void setActiveClip(MidiPattern* clip) {
        activeClip = clip;
        if (clip != nullptr) {
            double newScroll = clip->getStartX() * hZoom;
            hBar.setCurrentRange(newScroll, (double)getWidth() - keyW - vBarWidth);
        }
        repaint();
    }
    MidiPattern* getActiveClip() const { return activeClip; }

    void setPlaying(bool p) { isPlaying = p; }
    bool getIsPlaying() const { return isPlaying; }

    float getPlayheadPos() const { return playheadAbsPos; }
    void setPlayheadPos(float newPos) { playheadAbsPos = newPos; }

    float getLoopEndPos() const;

    double getBpm() const { return bpmValue; }
    void setBpm(double newBpm) { bpmValue = newBpm; repaint(); }

    int getPreviewPitch() const { return previewPitch; }
    bool getIsPreviewing() const { return isPreviewingNote; }

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    void timerCallback() override;

    void drawMinimap(juce::Graphics& g, juce::Rectangle<int> bounds);
    double getContentLengthTicks() const;

    const std::vector<Note>& getNotes() const {
        static std::vector<Note> empty;
        return (activeClip != nullptr) ? activeClip->getNotes() : empty;
    }

private:
    MidiPattern* activeClip = nullptr;

    float playheadAbsPos = 0.0f;
    bool isPlaying = false;
    double bpmValue = 120.0;

    int previewPitch = -1;
    bool isPreviewingNote = false;

    ToolMode currentTool = ToolMode::Draw;
    bool isSelecting = false;
    bool isDraggingPlayhead = false;
    bool isResizingAutoPanel = false;

    juce::Point<int> selectionStart;
    juce::Point<int> selectionEnd;
    juce::Point<int> currentMousePos{ -1, -1 };
    std::vector<FadingEntity> animations;

    std::set<int> selectedNotes;
    std::vector<NoteDragState> dragStates;
    bool isResizing = false;
    float dragStartAbsX = 0;
    int dragStartPitch = 0;
    int lastNoteWidth = 80;

    std::vector<std::vector<Note>> undoHistory;
    int currentHistoryIndex = -1;
    bool hasStateChanged = false;
    std::vector<Note> clipboard;
    bool isAltDragging = false;

    void commitHistory(); void undo(); void redo();
    void copySelectedNotes(); void pasteNotes(); void quantizeNotes();

    const int keyW = 80;
    const int toolH = 50;
    const int timelineH = 24;
    const int totalNotes = 128;
    const int vBarWidth = 32;
    const int navigatorH = 51;
    const float baseRowH = 22.0f;

    double hZoom = 1.0;
    double vZoom = 1.0;
    float getRowHeight() const { return baseRowH * vZoom; }

    juce::ComboBox snapSelector;
    juce::TextButton toolBtn;
    juce::TextButton pyHumanizeBtn;
    juce::TextButton chordGeneratorBtn;
    juce::TextButton linkAutoBtn;
    PlaylistNavigator hBar; VerticalNavigator vBar;
    juce::ComboBox rootNoteCombo; juce::ComboBox scaleCombo;

    double getSnapPixels();
    void updateScrollBars();
    int yToPitch(int y); int pitchToY(int pitch);
    int getNoteAt(int x, int y); bool isWhiteKey(int midiNote);

    void updateScale(); bool isNoteInScale(int midiPitch) const;
    void processPythonHumanize();
    void processTextToChord();
    int currentRootNoteOffset = 0; std::vector<int> currentScaleIntervals; std::vector<int> currentScaleNotesInOctave;
    juce::String getNoteName(int midiPitch) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};