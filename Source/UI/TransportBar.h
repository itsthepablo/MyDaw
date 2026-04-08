#pragma once
#include <JuceHeader.h>

class TransportBar : public juce::Component {
public:
    TransportBar() {}
    void resized() override {}
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};