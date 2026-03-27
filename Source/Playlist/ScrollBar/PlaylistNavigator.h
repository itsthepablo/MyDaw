#pragma once
#include <JuceHeader.h>
#include <functional>

class PlaylistNavigator : public juce::Component, private juce::Timer
{
public:
    PlaylistNavigator();
    ~PlaylistNavigator() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void timerCallback() override;

    void setRangeLimits(double minimum, double maximum);
    void setCurrentRange(double newStart, double newSize);
    double getCurrentRangeStart() const { return currentStart; }

    void setZoomContext(double currentZoom, double baseWidth);

    std::function<void(double)> onScrollMoved;
    std::function<void(double, double)> onZoomChanged;
    std::function<void(juce::Graphics&, juce::Rectangle<int>)> onDrawMinimap;

private:
    double totalMin = 0.0;
    double totalMax = 1.0;
    double currentStart = 0.0;
    double visibleSize = 1.0;

    double currentZoom = 1.0;
    double baseTotalWidth = 10240.0;

    enum DragMode { None, DraggingThumb, DraggingLeftEdge, DraggingRightEdge, HoldingLeftBtn, HoldingRightBtn };
    DragMode dragMode = None;

    double dragStartMouseX = 0.0;
    double dragStartThumbX = 0.0;
    double dragStartZoom = 1.0;
    int dragStartThumbLeft = 0;
    int dragStartThumbRight = 0;

    int targetMouseX = -1; // NUEVO: Para saber donde frenar el bloque

    const int btnSize = 20;

    juce::Rectangle<int> getTrackBounds() const;
    juce::Rectangle<int> getThumbBounds() const;
    juce::Rectangle<int> getLeftButtonBounds() const;
    juce::Rectangle<int> getRightButtonBounds() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistNavigator)
};