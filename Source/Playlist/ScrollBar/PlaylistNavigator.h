#pragma once
#include <JuceHeader.h>
#include <functional>

class PlaylistNavigator : public juce::Component
{
public:
    PlaylistNavigator();
    ~PlaylistNavigator() override;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void setRangeLimits(double minimum, double maximum);
    void setCurrentRange(double newStart, double newSize);
    double getCurrentRangeStart() const { return currentStart; }

    // Callback para avisarle al Playlist que la barra se movió
    std::function<void(double)> onScrollMoved;

private:
    double totalMin = 0.0;
    double totalMax = 1.0;
    double currentStart = 0.0;
    double visibleSize = 1.0;

    bool isDraggingThumb = false;
    double dragStartMouseX = 0.0;
    double dragStartThumbX = 0.0;

    juce::Rectangle<int> getThumbBounds() const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlaylistNavigator)
};