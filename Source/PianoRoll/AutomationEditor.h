#pragma once
#include <JuceHeader.h>
#include <set>
#include <map>
#include "../Data/GlobalData.h" 
#include "../Clips/Midi/MidiPattern.h"

// NO TOCAR NI SIMPLIFICAR HASTA QUE YO LO ORDENE
struct LinkedNode {
    int laneId;
    int nodeIdx;
    float startX;
};

enum class AutoDrawTool { Pencil, SCurve, Arc, Saw, Triangle, Square };

class AutomationEditor : public juce::Component, public juce::Button::Listener {
public:
    AutomationEditor();
    ~AutomationEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseMove(const juce::MouseEvent& event) override;

    void buttonClicked(juce::Button* button) override;

    float getVolumeAt(float playheadX);
    float getPanAt(float playheadX);
    float getFilterAt(float playheadX);
    float getPitchAt(float playheadX);

    void updateView(double hS, double hZ, float vS, float vZ, double snap, float ph);

    // <-- AQUÍ ESTÁ LA FUNCIÓN QUE EL COMPILADOR NO ENCUENTRA. ASEGÚRATE DE PEGAR ESTO:
    void setClipReference(MidiPattern* clip);

    void grabNodesUnderNotes(const std::set<int>& selectedIndices);
    void moveLinkedNodes(float deltaX);
    void clearLinkedNodes();

private:
    AutoLane dummyLane;

    juce::ComboBox autoSelector;
    juce::TextButton pencilBtn, lineBtn, arcBtn, sawBtn, triBtn, sqrBtn;
    std::vector<juce::TextButton*> toolButtons;

    AutoDrawTool currentDrawTool = AutoDrawTool::Pencil;
    int radioGroupId = 99;

    int draggedAutoNodeIdx = -1;
    int draggedTensionNodeIdx = -1;
    bool isDraggingNode = false;
    bool isMouseOverPanel = false;

    juce::Point<int> mouseDownPos;
    juce::Point<int> currentMousePos;
    int currentPreviewCellIdx = -1;

    float nodeOriginalX = 0.0f;
    float copiedNodeValue = -1.0f;

    juce::Point<float> tooltipPos;
    float tooltipValue = 0.0f;

    juce::CriticalSection dataLock;
    MidiPattern* clipRef = nullptr;
    std::vector<LinkedNode> linkedNodes;

    const int keyW = 80;
    const int toolbarH = 35;

    double hScroll = 0.0f;
    double hZoom = 1.0f;
    float vScroll = 0.0f;
    float vZoom = 1.0f;
    double snapPixels = 80.0;
    float currentPlayheadX = 0.0f;

    float autoVZoom = 1.0f;

    AutoLane* getCurrentAutoLane();
    AutoLane* getLaneById(int id);
    juce::Colour getLaneColor();
    juce::String getFormattedValue(float val);

    float getYForValue(float val);
    float getValueForY(float y);
    void drawShapeAt(float rawAx, float vy);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationEditor)
};