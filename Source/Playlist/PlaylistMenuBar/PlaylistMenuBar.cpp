#include "PlaylistMenuBar.h"

PlaylistMenuBar::PlaylistMenuBar()
{
    addAndMakeVisible(btnUndo);
    addAndMakeVisible(btnRedo);
    
    addAndMakeVisible(btnPointer);
    addAndMakeVisible(btnScissor);
    addAndMakeVisible(btnEraser);
    addAndMakeVisible(btnMute);

    btnPointer.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    btnScissor.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    btnEraser.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    btnMute.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);

    btnUndo.onClick = [this] { if (onUndo) onUndo(); };
    btnRedo.onClick = [this] { if (onRedo) onRedo(); };

    btnPointer.onClick = [this] { if (onToolChanged) onToolChanged(1); };
    btnScissor.onClick = [this] { if (onToolChanged) onToolChanged(3); };
    btnEraser.onClick = [this] { if (onToolChanged) onToolChanged(4); };
    btnMute.onClick = [this] { if (onToolChanged) onToolChanged(5); };

    addAndMakeVisible(snapBox);
    snapBox.addItem("Bar", 1);
    snapBox.addItem("Beat", 2);
    snapBox.addItem("1/2 Beat", 3);
    snapBox.addItem("1/4 Beat", 4);
    snapBox.addItem("None", 5);
    snapBox.setSelectedId(2, juce::dontSendNotification);

    snapBox.onChange = [this] {
        if (!onSnapChanged) return;
        int id = snapBox.getSelectedId();
        double pixelsPerBeat = 80.0;
        if (id == 1) onSnapChanged(pixelsPerBeat * 4.0); // Bar (Assuming 4/4)
        else if (id == 2) onSnapChanged(pixelsPerBeat); // Beat
        else if (id == 3) onSnapChanged(pixelsPerBeat / 2.0); // 1/2 Beat
        else if (id == 4) onSnapChanged(pixelsPerBeat / 4.0); // 1/4 Beat
        else if (id == 5) onSnapChanged(1.0); // None (1 pixel)
    };
}

PlaylistMenuBar::~PlaylistMenuBar() {}

void PlaylistMenuBar::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(35, 37, 40)); 
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());
}

void PlaylistMenuBar::resized()
{
    auto area = getLocalBounds().reduced(2);
    
    // Herramientas
    int btnW = 56;
    btnPointer.setBounds(area.removeFromLeft(btnW).reduced(2));
    btnScissor.setBounds(area.removeFromLeft(btnW).reduced(2));
    btnEraser.setBounds(area.removeFromLeft(btnW).reduced(2));
    btnMute.setBounds(area.removeFromLeft(btnW).reduced(2));

    area.removeFromLeft(10); // Spacing

    // Snap
    snapBox.setBounds(area.removeFromLeft(100).reduced(2));

    area.removeFromLeft(10); // Spacing

    // Undo/Redo
    btnUndo.setBounds(area.removeFromLeft(56).reduced(2));
    btnRedo.setBounds(area.removeFromLeft(56).reduced(2));
}