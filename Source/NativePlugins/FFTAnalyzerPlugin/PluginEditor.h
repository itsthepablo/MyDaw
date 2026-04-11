#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class TpAnalyzerAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::Timer
{
public:
    TpAnalyzerAudioProcessorEditor(TpAnalyzerAudioProcessor&);
    ~TpAnalyzerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

private:
    TpAnalyzerAudioProcessorEditor() = delete;
    TpAnalyzerAudioProcessor& audioProcessor;

    // Vectores para los datos visuales
    std::vector<float> displaySpectrum;
    std::vector<float> displayPeak;

    // Controles limpios
    juce::TextButton masterButton;
    juce::TextButton defaultButton;

    int mouseX = -1;
    int mouseY = -1;

    // OPTIMIZACIÓN: Caché para el fondo estático
    juce::Image backgroundCache;
    void bakeBackground(); // Función para pre-renderizar el grid

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TpAnalyzerAudioProcessorEditor)
};