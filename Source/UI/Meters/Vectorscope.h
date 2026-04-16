#pragma once
#include <JuceHeader.h>
#include "../../Data/Track.h"

// ==============================================================================
// VECTORSCOPIO / GONIÓMETRO PROFESIONAL (ESTILO TP INSPECTOR)
// ==============================================================================
class Vectorscope : public juce::Component, public juce::Timer
{
public:
    Vectorscope();
    ~Vectorscope() override;

    void setTrack(Track* t) { track = t; }

    void paint(juce::Graphics& g) override;
    void timerCallback() override;

private:
    void updatePoints();
    void drawBackground(juce::Graphics& g, juce::Rectangle<int> bounds);

    Track* track = nullptr;

    struct VectorPoint {
        float x = 0;
        float y = 0;
    };

    static const int maxPoints = 512;
    VectorPoint points[maxPoints];
    int head = 0;

    juce::Colour scopeColor = juce::Colours::cyan;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Vectorscope)
};
