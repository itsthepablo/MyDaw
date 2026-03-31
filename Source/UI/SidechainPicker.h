#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"

// ==============================================================================
// SidechainPicker — Un ComboBox personalizado para elegir la fuente de Sidechain
// ==============================================================================
class SidechainPicker : public juce::ComboBox {
public:
    SidechainPicker(std::function<void(int)> onSourceChanged)
        : sourceChangedCallback(onSourceChanged)
    {
        setEditableText(false);
        setJustificationType(juce::Justification::centred);
        setTextWhenNothingSelected("No Sidechain");
        setTextWhenNoChoicesAvailable("No Tracks");
        
        onChange = [this] {
            if (sourceChangedCallback)
                sourceChangedCallback(getSelectedId() - 2); // -1 = None, 0+ = Track ID
        };
        
        // Estilo oscuro y compacto
        setColour(juce::ComboBox::backgroundColourId, juce::Colour(30, 33, 36));
        setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentWhite);
    }

    void refresh(Track* currentTrack, const juce::Array<Track*>& allTracks, int currentSourceId)
    {
        clear(juce::dontSendNotification);
        addItem("None", 1);
        
        int i = 2;
        for (auto* t : allTracks) {
            if (t != currentTrack) { // Evitar feedback loop simple
                addItem(juce::String(t->getId()) + ": " + t->getName(), t->getId() + 2);
            }
        }
        
        if (currentSourceId == -1) setSelectedId(1, juce::dontSendNotification);
        else setSelectedId(currentSourceId + 2, juce::dontSendNotification);
    }

private:
    std::function<void(int)> sourceChangedCallback;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidechainPicker)
};
