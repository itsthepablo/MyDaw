#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include "../UI/LevelMeter.h"
#include "../UI/Knobs/FloatingValueSlider.h"

// ==============================================================================
// MasterTrackStrip — Sidebar vertical de Master
//
// Rediseñado para ser un sidebar vertical a la derecha.
// Incluye Paneo (top), Mute/Solo/FX y un Slider de Volumen sobre el Medidor.
// ==============================================================================
class MasterTrackStrip : public juce::Component
{
public:
    // Callbacks hacia MainComponent
    std::function<void(Track*)> onTrackSelected;

    MasterTrackStrip();
    ~MasterTrackStrip() override;

    void setMasterTrack(Track* t);
    Track* getMasterTrack() const;

    void setSelected(bool selected) { isSelected = selected; repaint(); }
    bool getSelected() const { return isSelected; }

    // Eliminado timerCallback - LevelMeter se auto-actualiza
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    Track* masterTrack = nullptr;
    bool isSelected = false;

    juce::Label      masterLabel;
    juce::TextButton muteBtn, soloBtn, effectsBtn;
    FloatingValueSlider panKnob;
    juce::Slider        volSlider; 
    LevelMeter       levelMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterTrackStrip)
};