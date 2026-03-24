#pragma once

#include <JuceHeader.h>
#include <atomic>
#include "SimpleLoudness.h"
#include "ExactMeter.h"
#include "VuMeter.h" 
#include "SimpleAnalyzer.h" 

class MiPrimerVSTAudioProcessor : public juce::AudioProcessor
{
public:
    MiPrimerVSTAudioProcessor();
    ~MiPrimerVSTAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioParameterChoice* modeSelector;
    const juce::StringArray modeOptions{ "Stereo", "Mono", "Side", "Invert", "Swap", "Offset", "L Solo", "R Solo" };

    juce::AudioParameterBool* btnLow;
    juce::AudioParameterBool* btnMid;
    juce::AudioParameterBool* btnHigh;

    juce::AudioParameterBool* btnW;
    juce::AudioParameterBool* btnPhase;
    juce::AudioParameterBool* btnBypass;

    // --- NUEVO: BOTÓN DE MODO FFT ---
    juce::AudioParameterBool* btnFftMaster;

    juce::AudioParameterFloat* vuCalibParam;

    std::atomic<float> rmsLevelLeft{ 0.0f };
    std::atomic<float> rmsLevelRight{ 0.0f };
    std::atomic<float> peakLevelLeft{ 0.0f };
    std::atomic<float> peakLevelRight{ 0.0f };

    std::atomic<float> lufsShort{ -100.0f };
    std::atomic<float> lufsIntegrated{ -100.0f };
    std::atomic<float> lufsRange{ 0.0f };
    std::atomic<float> lufsTruePeak{ -100.0f };

    std::atomic<float> vuLevelL{ -20.0f };
    std::atomic<float> vuLevelR{ -20.0f };
    std::atomic<float> vuPeakL{ -20.0f };
    std::atomic<float> vuPeakR{ -20.0f };

    std::atomic<float> currentCorrelation{ 0.0f };

    static const int scopeSize = 1024;
    float scopeBufferL[scopeSize] = { 0.0f };
    float scopeBufferR[scopeSize] = { 0.0f };
    int scopeWriteIndex = 0;

    int windowWidth = 181;
    int windowHeight = 703;

    int wWindowX = -1;
    int wWindowY = -1;
    int wWindowWidth = 140;
    int wWindowHeight = 140;

    juce::Colour savedScopeColor = juce::Colours::deeppink;
    juce::Colour savedMeterLow = juce::Colours::limegreen;
    juce::Colour savedMeterMid = juce::Colours::yellow;
    juce::Colour savedMeterHigh = juce::Colours::red;
    juce::Colour savedCorrelationPosColor = juce::Colours::limegreen;

    int savedVectorscopeMode = 0;

    SimpleAnalyzer spectrumAnalyzer;

    void resetLoudnessMeters() {
        lufsEngine.reset();
        meterL.reset();
        meterR.reset();
    }

private:
    juce::IIRFilter lowBandFilter[2];
    juce::IIRFilter highBandFilter[2];
    juce::IIRFilter midLowCut[2];
    juce::IIRFilter midHighCut[2];

    AnalyzerLoudness lufsEngine; // --- CORRECCIÓN ---
    ExactMeter meterL;
    ExactMeter meterR;

    VuMeterDSP vuDSP_L;
    VuMeterDSP vuDSP_R;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MiPrimerVSTAudioProcessor)
};