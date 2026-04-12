#pragma once
#include <JuceHeader.h>
#include "../../../Mixer/UI/MixerChannelUI.h"
#include "../../../Data/Track.h"

/**
 * SelectedTrackPanel: El contenido del dock derecho.
 * Muestra una vista completa de canal de mixer para el track seleccionado.
 */
class SelectedTrackPanel : public juce::Component, private juce::Timer {
public:
    SelectedTrackPanel() {
        setOpaque(true);
        startTimerHz(30);
    }

    ~SelectedTrackPanel() override {
        stopTimer();
        channelUI.reset();
    }

    void timerCallback() override {
        if (channelUI != nullptr && isShowing()) {
            channelUI->updateUI();
        }
    }

    /**
     * Asigna el track a visualizar. Si es el mismo de antes, solo actualiza.
     */
    void setTrack(Track* newTrack) {
        if (currentTrack == newTrack && channelUI != nullptr) {
            channelUI->updateUI();
            return;
        }

        currentTrack = newTrack;
        channelUI.reset();

        if (currentTrack != nullptr) {
            // Creamos una nueva instancia del canal. No estamos en modo "mini"
            channelUI = std::make_unique<MixerChannelUI>(currentTrack, false);
            
            // Replicar los callbacks necesarios para que el canal funcione
            channelUI->onAddVST3 = onAddVST3;
            channelUI->onAddNativeUtility = onAddNativeUtility;
            channelUI->onOpenPlugin = onOpenPlugin;
            channelUI->onDeleteEffect = onDeleteEffect;
            channelUI->onBypassChanged = onBypassChanged;
            channelUI->onAddSend = onAddSend;
            channelUI->onDeleteSend = onDeleteSend;
            channelUI->onCreateAutomation = onCreateAutomation;

            addAndMakeVisible(*channelUI);
            channelUI->updateUI();
        }
        
        resized();
        repaint();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(25, 27, 30));
        
        if (currentTrack == nullptr) {
            g.setColour(juce::Colours::grey.withAlpha(0.3f));
            g.setFont(14.0f);
            g.drawText("No Track Selected", getLocalBounds(), juce::Justification::centred);
        }
    }

    void resized() override {
        if (channelUI != nullptr) {
            channelUI->setBounds(getLocalBounds());
        }
    }

    // Callbacks que deben conectarse desde MainComponent (igual que el Mixer principal)
    std::function<void(Track&)> onAddVST3, onAddNativeUtility, onAddSend;
    std::function<void(Track&, int)> onOpenPlugin, onDeleteEffect, onDeleteSend;
    std::function<void(Track&, int, bool)> onBypassChanged;
    std::function<void(Track&, int, juce::String)> onCreateAutomation;

private:
    Track* currentTrack = nullptr;
    std::unique_ptr<MixerChannelUI> channelUI;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SelectedTrackPanel)
};
