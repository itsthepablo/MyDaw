#pragma once
#include <JuceHeader.h>

class TransportBridge {
public:
    static void connect(juce::TextButton& playBtn, 
                        PianoRollComponent& pianoRoll, 
                        PlaylistComponent& playlist) 
    {
        playBtn.onClick = [&playBtn, &pianoRoll, &playlist] {
            bool newState = !pianoRoll.getIsPlaying();
            
            pianoRoll.setPlaying(newState);
            playlist.isPlaying = newState;
            
            if (newState) {
                pianoRoll.setPlayheadPos(0);
                playlist.setPlayheadPos(0);
            }
            
            playBtn.setButtonText(newState ? "STOP" : "PLAY");
        };
    }
};