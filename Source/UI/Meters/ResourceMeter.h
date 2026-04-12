#pragma once
#include <JuceHeader.h>
#include "../../Engine/Routing/PDCManager.h" 

/**
 * ResourceMeter — Monitor de rendimiento (CPU, RAM, Latencia)
 */
class ResourceMeter : public juce::Component, private juce::Timer {
public:
    ResourceMeter(juce::AudioAppComponent& app);
    ~ResourceMeter() override;

    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;

    juce::AudioAppComponent& audioApp;

    double dspLoad = 0.0;
    int ramLoadMB = 0;
    int bufferSize = 0;
    double sampleRate = 0.0;
    double latencyMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResourceMeter)
};