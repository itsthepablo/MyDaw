#pragma once
#include <JuceHeader.h>
#include <vector>
#include "ChannelRackRow.h"

// ==============================================================================
// 4. COMPONENTE PRINCIPAL DEL RACK
// ==============================================================================
class ChannelRackPanel : public juce::Component {
public:
    ChannelRackPanel();
    ~ChannelRackPanel() override = default;

    void rebuildRows();
    void paint(juce::Graphics& g) override;
    void lookAndFeelChanged() override { repaint(); }
    void resized() override;

private:
    std::vector<RackChannel> globalChannels;

    juce::Viewport viewport;
    juce::Component contentComponent;
    juce::OwnedArray<ChannelRackRow> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackPanel)
};