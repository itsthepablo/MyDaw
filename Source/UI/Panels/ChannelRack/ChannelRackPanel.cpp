#include "ChannelRackPanel.h"
#include "ChannelRackRow.h"

// ==============================================================================
// CHANNEL RACK ROW IMPLEMENTATION
// ==============================================================================

void ChannelRackRow::paint(juce::Graphics& g) {
    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
        g.fillAll(theme->getSkinColor("CHANNEL_RACK_BG", juce::Colour(30, 32, 35)).brighter(0.1f));
    } else {
        g.fillAll(juce::Colour(45, 48, 52));
    }
    
    g.setColour(juce::Colour(30, 32, 35));
    g.drawHorizontalLine(getHeight() - 1, 0, (float)getWidth());

    g.setColour(muteBtn.getToggleState() ? juce::Colours::limegreen : juce::Colour(60, 60, 60));
    g.fillEllipse(8, (float)getHeight() / 2 - 4, 8, 8);

    juce::Rectangle<int> nameBounds(85, 4, 100, getHeight() - 8);
    g.setColour(channel.color);
    g.fillRoundedRectangle(nameBounds.toFloat(), 3.0f);

    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawHorizontalLine(nameBounds.getY() + 1, (float)nameBounds.getX() + 2, (float)nameBounds.getRight() - 2);

    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText(channel.name, nameBounds.reduced(8, 0), juce::Justification::centredLeft, true);
}

void ChannelRackRow::resized() {
    auto area = getLocalBounds();

    muteBtn.setBounds(area.removeFromLeft(20));
    area.removeFromLeft(5);
    panSlider.setBounds(area.removeFromLeft(24).reduced(0, 4));
    area.removeFromLeft(4);
    volSlider.setBounds(area.removeFromLeft(24).reduced(0, 4));

    area.removeFromLeft(110);

    int numSteps = steps.size();
    float stepW = (float)area.getWidth() / numSteps;
    float currentX = (float)area.getX();

    for (auto* step : steps) {
        step->setBounds((int)currentX, area.getY(), (int)stepW, area.getHeight());
        currentX += stepW;
    }
}

// ==============================================================================
// CHANNEL RACK PANEL IMPLEMENTATION
// ==============================================================================

ChannelRackPanel::ChannelRackPanel() {
    globalChannels.push_back({ "Kick", 0.8f, 0.0f, false, {0}, juce::Colour(90, 100, 110) });
    globalChannels.push_back({ "Clap", 0.8f, 0.0f, false, {0}, juce::Colour(110, 90, 100) });
    globalChannels.push_back({ "HiHat", 0.7f, 0.0f, false, {0}, juce::Colour(100, 110, 90) });
    globalChannels.push_back({ "Snare", 0.8f, 0.0f, false, {0}, juce::Colour(95, 95, 105) });

    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&contentComponent, false);
    viewport.setScrollBarsShown(true, false, true, false);

    rebuildRows();
}

void ChannelRackPanel::rebuildRows() {
    contentComponent.removeAllChildren();
    rows.clear();

    for (auto& ch : globalChannels) {
        auto* newRow = rows.add(new ChannelRackRow(ch));
        contentComponent.addAndMakeVisible(newRow);
    }
    resized();
}

void ChannelRackPanel::paint(juce::Graphics& g) {
    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
        g.fillAll(theme->getSkinColor("CHANNEL_RACK_BG", juce::Colour(30, 32, 35)));
    } else {
        g.fillAll(juce::Colour(30, 32, 35));
    }
}

void ChannelRackPanel::resized() {
    viewport.setBounds(getLocalBounds());

    int rowHeight = 36;
    int totalHeight = (int)globalChannels.size() * rowHeight;

    int contentW = viewport.getWidth() - (viewport.isVerticalScrollBarShown() ? viewport.getScrollBarThickness() : 0);
    contentComponent.setBounds(0, 0, contentW, totalHeight);

    for (int i = 0; i < rows.size(); ++i) {
        rows[i]->setBounds(0, i * rowHeight, contentComponent.getWidth(), rowHeight);
    }
}
