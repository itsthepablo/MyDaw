#pragma once
#include <JuceHeader.h>
#include "../Knobs/FloatingValueBox.h"
#include "../../Data/Track.h"
#include "../../Mixer/Bridges/MixerParameterBridge.h"
#include "../../Mixer/LookAndFeel/MixerLevelMeterLF.h"

/**
 * LevelMeter — Componente Unificado Hi-Fi
 */
class LevelMeter : public juce::Component,
    public juce::SettableTooltipClient,
    private juce::Timer
{
public:
    enum MeterSource { Stereo, Mid, Side };

    LevelMeter(Track* t = nullptr);
    ~LevelMeter() override;

    void setTrack(Track* t);
    void setSource(MeterSource s);
    void reset();

    void setLevel(float left, float right);
    void setHorizontal(bool horizontal);

    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;

    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;
    void updateValueBoxText();
    void positionValueBox();

    Track* track = nullptr;
    MeterSource source = Stereo;
    
    float dispL = 0.0f, dispR = 0.0f;
    float peakL = 0.0f, peakR = 0.0f;
    int holdL = 0, holdR = 0;
    bool isClipping = false;
    bool isMouseOverMeter = false;
    bool isHorizontal = false;

    float absoluteMaxPeakL = 0.0f;
    float absoluteMaxPeakR = 0.0f;
    
    FloatingValueBox valueBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};