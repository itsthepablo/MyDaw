#pragma once
#include <JuceHeader.h>
#include <functional>

class VerticalNavigator : public juce::Component, private juce::Timer
{
public:
    VerticalNavigator();
    ~VerticalNavigator() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void timerCallback() override;

    void setRangeLimits(double minimum, double maximum);
    void setCurrentRange(double newStart, double newSize);
    double getCurrentRangeStart() const { return currentStart; }
    double getCurrentRangeSize() const { return visibleSize; }
    double getMaximumRangeLimit() const { return totalMax; }

    std::function<void(double)> onScrollMoved;

private:
    double totalMin = 0.0;
    double totalMax = 1.0;
    double currentStart = 0.0;
    double visibleSize = 1.0;

    enum DragMode { None, DraggingThumb, HoldingUpBtn, HoldingDownBtn };
    DragMode dragMode = None;

    double dragStartMouseY = 0.0;
    double dragStartThumbY = 0.0;

    int targetMouseY = -1; // NUEVO: Para frenar el bloque vertical

    const int btnSize = 20;

    juce::Rectangle<int> getTrackBounds() const;
    juce::Rectangle<int> getThumbBounds() const;
    juce::Rectangle<int> getUpButtonBounds() const;
    juce::Rectangle<int> getDownButtonBounds() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VerticalNavigator)
};