#include "UIManager.h"

void UIManager::setupUI(DAWUIComponents& ui, juce::Component& parent, std::function<void()> onClosePianoRoll) {
    parent.addAndMakeVisible(ui.topMenuBar);
    parent.addAndMakeVisible(ui.hintPanel);
    parent.addAndMakeVisible(ui.transportBar);
    
    if (ui.resourceMeter != nullptr)
        parent.addAndMakeVisible(*ui.resourceMeter);

    parent.addAndMakeVisible(ui.leftSidebar);
    parent.addAndMakeVisible(ui.sidebarResizer);
    parent.addAndMakeVisible(ui.trackContainer);
    parent.addAndMakeVisible(ui.playlistUI);
    parent.addAndMakeVisible(ui.bottomDockResizer);
    parent.addAndMakeVisible(ui.bottomDock);
    parent.addAndMakeVisible(ui.mixerUI);
    parent.addAndMakeVisible(ui.masterChannelUI);
    parent.addAndMakeVisible(ui.pianoRollUI);
    parent.addAndMakeVisible(ui.closePianoRollBtn);

    // --- MASTER TRACK ---
    ui.masterTrackObj = std::make_unique<Track>(0, "Master", TrackType::Audio);
    ui.masterStrip.setMasterTrack(ui.masterTrackObj.get());
    parent.addAndMakeVisible(ui.masterStrip);

    ui.pianoRollUI.setVisible(false);
    ui.closePianoRollBtn.setVisible(false);
    ui.mixerUI.setVisible(false);
    ui.masterChannelUI.setVisible(false);
    ui.masterStrip.setVisible(true);

    ui.closePianoRollBtn.setButtonText("Cerrar Piano Roll");
    ui.closePianoRollBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(200, 70, 70));
    ui.closePianoRollBtn.onClick = onClosePianoRoll;

    // --- CONFIGURACIÓN MINI MIXER ---
    ui.miniMixerUI.isMiniMixer = true;

    // --- ENRUTAMIENTO DE BOTONES ---
    ui.topMenuBar.onToggleMixer = [&ui] {
        if (!ui.bottomDock.isVisible()) {
            ui.bottomDock.setVisible(true);
            ui.bottomDock.showTab(BottomDock::MixerTab);
        } else {
            if (ui.bottomDock.getCurrentTab() == BottomDock::MixerTab) {
                ui.bottomDock.setVisible(false);
            } else {
                ui.bottomDock.showTab(BottomDock::MixerTab);
            }
        }
        if (ui.onResized) ui.onResized();
    };

    ui.bottomDock.setVisible(true);
    ui.leftSidebar.setVisible(true);
}