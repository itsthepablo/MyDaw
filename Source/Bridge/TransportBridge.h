#pragma once
#include <JuceHeader.h>
#include "../UI/TransportBar.h"
#include "../UI/TopMenuBar.h"
#include "../PianoRoll/PianoRollComponent.h"
#include "../Playlist/PlaylistComponent.h"

class TransportBridge {
public:
    static void connect(TransportBar& bar,
        TopMenuBar& topMenu,
        PianoRollComponent& pianoRoll,
        PlaylistComponent& playlist)
    {
        topMenu.playBtn.onClick = [&topMenu, &pianoRoll, &playlist] {
            bool newState = !pianoRoll.getIsPlaying();

            pianoRoll.setPlaying(newState);
            playlist.isPlaying = newState;

            if (newState) {
                pianoRoll.setPlayheadPos(0);
                playlist.setPlayheadPos(0);
            }

            topMenu.playBtn.setButtonText(newState ? "S" : "P");
            topMenu.playBtn.setColour(juce::TextButton::buttonColourId,
                newState ? juce::Colours::red : juce::Colours::green);
            };

        bar.bpmSlider.onValueChange = [&bar, &pianoRoll, &playlist] {
            double val = bar.bpmSlider.getValue();
            bar.bpmLabel.setText(juce::String(val, 1) + " BPM", juce::dontSendNotification);

            pianoRoll.setBpm(val);
            playlist.setBpm(val);
            };
    }
};