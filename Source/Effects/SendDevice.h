#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include <functional>

// ==============================================================================
// Componente que representa un ENVÍO (Send) en el panel de efectos
// ==============================================================================
class SendDevice : public juce::Component {
public:
    SendDevice(int sendIndex, Track* source, const juce::Array<Track*>& availableTracks, std::function<void()> onChange, std::function<void(int)> onDelete)
        : idx(sendIndex), sourceTrack(source), allTracks(availableTracks), onNotifyChange(onChange), onNotifyDelete(onDelete)
    {
        addAndMakeVisible(targetSelector);
        targetSelector.addItem("None", 1000);
        for (auto* t : allTracks) {
            if (t != sourceTrack) 
                targetSelector.addItem(t->getName(), t->getId());
        }

        if (idx < (int)sourceTrack->sends.size()) {
            auto& s = sourceTrack->sends[idx];
            targetSelector.setSelectedId(s.targetTrackId == -1 ? 1000 : s.targetTrackId, juce::dontSendNotification);
            gainSlider.setValue(s.gain, juce::dontSendNotification);
            prePostBtn.setButtonText(s.isPreFader ? "PRE" : "POST");
            prePostBtn.setToggleState(s.isPreFader, juce::dontSendNotification);
        }

        targetSelector.onChange = [this] {
            if (idx < (int)sourceTrack->sends.size()) {
                int id = targetSelector.getSelectedId();
                sourceTrack->sends[idx].targetTrackId = (id == 1000 ? -1 : id);
                if (onNotifyChange) onNotifyChange();
            }
        };

        addAndMakeVisible(gainSlider);
        gainSlider.setSliderStyle(juce::Slider::LinearBar);
        gainSlider.setRange(0.0, 1.5, 0.01);
        gainSlider.setTextValueSuffix("x");
        gainSlider.onValueChange = [this] {
            if (idx < (int)sourceTrack->sends.size()) {
                sourceTrack->sends[idx].gain = (float)gainSlider.getValue();
            }
        };

        addAndMakeVisible(prePostBtn);
        prePostBtn.setClickingTogglesState(true);
        prePostBtn.onClick = [this] {
            if (idx < (int)sourceTrack->sends.size()) {
                sourceTrack->sends[idx].isPreFader = prePostBtn.getToggleState();
                prePostBtn.setButtonText(sourceTrack->sends[idx].isPreFader ? "PRE" : "POST");
                if (onNotifyChange) onNotifyChange();
            }
        };

        // --- NUEVO: BOTÓN DE RUTEO (ST / MID / SIDE) ---
        addAndMakeVisible(routingBtn);
        updateRoutingVisuals();
        routingBtn.onClick = [this] {
            if (idx < (int)sourceTrack->sends.size()) {
                auto& s = sourceTrack->sends[idx];
                int r = (int)s.routing;
                s.routing = (SendRouting)((r + 1) % 3);
                updateRoutingVisuals();
                // No necesitamos relanzar topología porque el ruteo interno no cambia dependencias
            }
        };

        addAndMakeVisible(deleteBtn);
        deleteBtn.setButtonText("x");
        deleteBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(100, 30, 30));
        deleteBtn.onClick = [this] { if (onNotifyDelete) onNotifyDelete(idx); };
    }

    void updateRoutingVisuals() {
        if (idx >= (int)sourceTrack->sends.size()) return;
        auto& s = sourceTrack->sends[idx];
        
        switch (s.routing) {
            case SendRouting::Stereo:
                routingBtn.setButtonText("ST");
                routingBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
                break;
            case SendRouting::Mid:
                routingBtn.setButtonText("MID");
                routingBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::orange);
                break;
            case SendRouting::Side:
                routingBtn.setButtonText("SIDE");
                routingBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::yellow);
                break;
        }
    }

    void paint(juce::Graphics& g) override {
        auto area = getLocalBounds().reduced(2);
        g.setColour(juce::Colour(55, 60, 65));
        g.fillRoundedRectangle(area.toFloat(), 6.0f);
        
        g.setColour(juce::Colours::orange.withAlpha(0.7f));
        g.setFont(juce::Font(12.0f, juce::Font::bold));
        g.drawText("SEND", area.getX() + 5, area.getY() + 5, 40, 15, juce::Justification::centredLeft);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(8);
        area.removeFromTop(15);
        
        targetSelector.setBounds(area.removeFromTop(20));
        area.removeFromTop(5);
        gainSlider.setBounds(area.removeFromTop(20));
        area.removeFromTop(5);
        
        int bw = (area.getWidth() / 3) - 2;
        prePostBtn.setBounds(area.getX(), area.getY(), bw, 20);
        routingBtn.setBounds(area.getX() + bw + 2, area.getY(), bw, 20);
        deleteBtn.setBounds(area.getX() + (bw * 2) + 4, area.getY(), bw, 20);
    }

private:
    int idx;
    Track* sourceTrack;
    juce::Array<Track*> allTracks;
    std::function<void()> onNotifyChange;
    std::function<void(int)> onNotifyDelete;

    juce::ComboBox targetSelector;
    juce::Slider gainSlider;
    juce::TextButton prePostBtn;
    juce::TextButton routingBtn;
    juce::TextButton deleteBtn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendDevice)
};
