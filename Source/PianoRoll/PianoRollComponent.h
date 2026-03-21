#pragma once
#include <JuceHeader.h>
#include <vector>
#include <set>
#include "../GlobalData.h" 
#include "AutomationEditor.h" 

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
    private juce::Timer,
    private juce::ScrollBar::Listener
{
public:
    PianoRollComponent();
    ~PianoRollComponent() override;

    void setActiveNotes(std::vector<Note>* trackNotes) {
        externalNotes = trackNotes;
        automationEditor.setNotesReference(trackNotes);
        repaint();
    }
    const std::vector<Note>* getActiveNotesPointer() const { return externalNotes; }

    void setPlaying(bool p) { isPlaying = p; }
    bool getIsPlaying() const { return isPlaying; }

    float getPlayheadPos() const { return playheadAbsPos; }
    void setPlayheadPos(float newPos) { playheadAbsPos = newPos; }

    float getLoopEndPos() const;

    // Cambiado para usar el valor interno en lugar del slider eliminado
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

    void scrollBarMoved(juce::ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    void timerCallback() override;

    const std::vector<Note>& getNotes() const {
        static std::vector<Note> empty;
        return (externalNotes != nullptr) ? *externalNotes : empty;
    }

    AutomationEditor& getAutoEditor() { return automationEditor; }

private:
    std::vector<Note>* externalNotes = nullptr;
    float playheadAbsPos = 0.0f;
    bool isPlaying = false;
    double bpmValue = 120.0; // Nueva variable interna

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
    int autoH = 300;
    const int totalNotes = 128;
    const int scrollBarSize = 16;
    const float baseRowH = 22.0f;

    float hZoom = 1.0f;
    float vZoom = 1.0f;
    float getRowHeight() const { return baseRowH * vZoom; }

    juce::Slider hZoomSlider; juce::Slider vZoomSlider;
    juce::Label hZoomLabel; juce::Label vZoomLabel;
    juce::ComboBox snapSelector;
    juce::TextButton toolBtn;
    juce::TextButton linkAutoBtn;
    juce::ScrollBar hBar{ false }; juce::ScrollBar vBar{ true };
    juce::ComboBox rootNoteCombo; juce::ComboBox scaleCombo;

    AutomationEditor automationEditor;

    double getSnapPixels();
    void updateScrollBars();
    int yToPitch(int y); int pitchToY(int pitch);
    int getNoteAt(int x, int y); bool isWhiteKey(int midiNote);

    void updateScale(); bool isNoteInScale(int midiPitch) const;
    int currentRootNoteOffset = 0; std::vector<int> currentScaleIntervals; std::vector<int> currentScaleNotesInOctave;
    juce::String getNoteName(int midiPitch) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoRollComponent)
};