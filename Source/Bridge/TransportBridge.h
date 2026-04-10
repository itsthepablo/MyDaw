#pragma once
#include <JuceHeader.h>
#include "../UI/Bars/TopMenuBar/TopMenuBar.h"
#include "../PianoRoll/PianoRollComponent.h"
#include "../Playlist/PlaylistComponent.h"

class TransportBridge {
public:
    static void connect(TopMenuBar& topMenu,
        PianoRollComponent& pianoRoll,
        PlaylistComponent& playlist)
    {
        topMenu.playBtn.onClick = [&topMenu, &pianoRoll, &playlist] {
            bool isPlaying = pianoRoll.getIsPlaying();
            auto mods = juce::ModifierKeys::getCurrentModifiers();

            if (!isPlaying) {
                pianoRoll.setPlaying(true);
                playlist.isPlaying = true;
                topMenu.playBtn.setButtonText("S");
                topMenu.playBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
            }
            else {
                pianoRoll.setPlaying(false);
                playlist.isPlaying = false;

                if (!mods.isCtrlDown() && !mods.isCommandDown()) {
                    pianoRoll.setPlayheadPos(0);
                    playlist.setPlayheadPos(0);
                    if (playlist.onPlayheadSeekRequested) playlist.onPlayheadSeekRequested(0.0f);
                }

                topMenu.playBtn.setButtonText("P");
                topMenu.playBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            }
            };

        topMenu.stopBtn.onClick = [&topMenu, &pianoRoll, &playlist] {
            pianoRoll.setPlaying(false);
            playlist.isPlaying = false;
            pianoRoll.setPlayheadPos(0);
            playlist.setPlayheadPos(0);
            if (playlist.onPlayheadSeekRequested) playlist.onPlayheadSeekRequested(0.0f);

            topMenu.playBtn.setButtonText("P");
            topMenu.playBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
            };

        topMenu.bpmControl.onBpmChanged = [&pianoRoll, &playlist](float val) {
            pianoRoll.setBpm(val);
            playlist.setBpm(val);
            };

        topMenu.requestPlaybackTimeInSeconds = [&pianoRoll]() -> double {
            double bpm = pianoRoll.getBpm();
            double pixelsPerSec = (bpm / 60.0) * 80.0;
            if (pixelsPerSec <= 0.0) return 0.0;
            return pianoRoll.getPlayheadPos() / pixelsPerSec;
            };
    }
};