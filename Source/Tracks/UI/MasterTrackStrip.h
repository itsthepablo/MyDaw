#pragma once
#include <JuceHeader.h>
#include "../Track.h"
#include "../../UI/LevelMeter.h"
#include "../../UI/Knobs/FloatingValueSlider.h"

// ==============================================================================
// MasterTrackStrip — Panel de Control del Master (Anclado al fondo)
//
// Rediseñado para ser una fila horizontal de 100px de altura,
// anclada bajo la lista de tracks y la playlist.
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

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

private:
    Track* masterTrack = nullptr;
    bool isSelected = false;

    juce::Label      masterLabel;
    juce::TextButton muteBtn, soloBtn, effectsBtn;
    FloatingValueSlider panKnob;
    FloatingValueSlider volKnob; 
    LevelMeter       levelMeter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterTrackStrip)
};