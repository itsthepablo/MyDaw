#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "PolarPatterns.h" 
#include "ModernDarkKnob.h"
#include "ScopeComponents.h"
#include "VuMeter.h" 

class MiPrimerVSTAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::Slider::Listener,
    public juce::Timer
{
public:
    MiPrimerVSTAudioProcessorEditor(MiPrimerVSTAudioProcessor&);
    ~MiPrimerVSTAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    void sliderValueChanged(juce::Slider* slider) override;

    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

private:
    MiPrimerVSTAudioProcessor& audioProcessor;

    // --- CORRECCIÓN: Eliminado openGLContext para compatibilidad con el DAW ---

    juce::Slider modeKnob;
    juce::Label modeDisplay;

    juce::TextButton vuButton{ "VU" };
    juce::TextButton balanceButton{ "B" };
    juce::TextButton fftModeButton{ "D" };

    int centralViewMode = 0;
    int vuViewMode = 0;

    juce::Slider vuCalibKnob;

    float uiVuL = -20.0f;
    float uiVuR = -20.0f;
    float uiVuPeakL = -20.0f;
    float uiVuPeakR = -20.0f;
    float uiVuCalib = 5.0f;

    bool isHoveringFft = false;
    juce::Point<float> fftCrosshairPos;

    juce::TextButton menuButton{ "" };

    juce::TextButton lowButton{ "L" };
    juce::TextButton midButton{ "M" };
    juce::TextButton highButton{ "H" };
    juce::TextButton phaseButton{ juce::String::fromUTF8("\xc3\x98") };
    juce::TextButton bypassButton{ "A/B" };

    std::unique_ptr<juce::ResizableCornerComponent> resizer;
    float scaleFactor = 1.0f;

    juce::Rectangle<int> rectHeaderBg;
    juce::Rectangle<int> rectKnob;
    juce::Rectangle<int> rectStats;
    juce::Rectangle<int> rectPeak;
    juce::Rectangle<int> rectRms;
    juce::Rectangle<int> rectOscilloscope;
    juce::Rectangle<int> rectVectorscope;
    juce::Rectangle<int> rectCorrelation;

    float txtPeakL = 0.0f; float txtPeakR = 0.0f;
    float txtRmsL = 0.0f; float txtRmsR = 0.0f;
    int rmsHoldCounterL = 0; int rmsHoldCounterR = 0;

    int scopeViewMode = 0;

    ScrollingWaveform waveformEngine;
    LegacyOscilloscope legacyScope;
    ScrollingPeakWaveform proLScopeEngine;

    juce::Colour scopeColor = juce::Colours::deeppink;
    juce::Colour meterLow = juce::Colours::limegreen;
    juce::Colour meterMid = juce::Colours::yellow;
    juce::Colour meterHigh = juce::Colours::red;
    juce::Colour correlationPosColor = juce::Colours::limegreen;

    PolarVectorscope polarEngine;
    ModernDarkKnob customKnob;

    juce::Path cachedVectorscopePath;

    void updateButtonColors();
    void updateModeText();
    void updateTopButtons();

    void drawLufsCentral(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawLufsStats(juce::Graphics& g, juce::Rectangle<int> bounds);
    void drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level, bool isRMS);

    void drawOscilloscope(juce::Graphics& g, int x, int y, int width, int height);
    void drawVectorscope(juce::Graphics& g, int x, int y, int width, int height);
    void drawFFT(juce::Graphics& g, juce::Rectangle<int> bounds);

    juce::Colour getMeterColor(float t);
    juce::String getDbString(float val);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiPrimerVSTAudioProcessorEditor)
};