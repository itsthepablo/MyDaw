#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <functional>
#include <vector>
#include "../../Data/Track.h"
#include "../../Mixer/Bridges/MixerParameterBridge.h"
#include "../../PluginHost/VSTHost.h" 
#include "../../UI/Knobs/FloatingValueSlider.h" 
#include "../../UI/Meters/LevelMeter.h"
#include "../../Modules/LoudnessTrack/UI/LoudnessMeter.h"
#include "../LookAndFeel/TrackLookAndFeel.h"

/**
 * TrackControlPanel — Componente visual para los controles de cada pista (Mute, Solo, Vol, Pan, etc.)
 * Refactorizado a .cpp para mejorar tiempos de compilación.
 */
class TrackControlPanel : public juce::Component, private juce::Timer, public juce::ChangeListener {
public:
    std::function<void()> onFxClick, onInstrumentClick, onPianoRollClick, onDeleteClick, onEffectsClick, onFolderStateChange;
    std::function<void()> onWaveformViewChanged, onTrackColorChanged;
    std::function<void(int)> onPluginClick;
    std::function<void(const juce::ModifierKeys&)> onTrackSelected;
    std::function<void()> onMoveToNewFolder;
    std::function<void(int, juce::String)> onCreateAutomation;

    int dragHoverMode = 0;
    Track& track;

    TrackControlPanel(Track& t);
    ~TrackControlPanel() override;

    void timerCallback() override;
    void updateFolderBtnVisuals();
    void updatePlugins();

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void changeListenerCallback(juce::ChangeBroadcaster* s) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& k) override;

private:
    void showAutomationMenu(int paramId, const juce::String& paramName);

    juce::Label nameLabel; 
    juce::TextButton muteBtn, soloBtn, midSoloBtn, sideSoloBtn; 
    FloatingValueSlider volKnob, panKnob;
    juce::TextButton fxButton, prButton, inlineBtn, effectsBtn, folderBtn, compactBtn;
    juce::ComboBox targetLufsCombo, balanceScaleCombo, midSideScaleCombo, midSideModeCombo;
    std::unique_ptr<LoudnessMeter> loudnessMeter;
    juce::OwnedArray<juce::TextButton> pluginButtons; 
    LevelMeter levelMeter;
    TrackLevelMeterLF trackMeterLF;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackControlPanel)
};
