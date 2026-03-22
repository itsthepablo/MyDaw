#pragma once
#include <JuceHeader.h>

class ResourceMeter : public juce::Component, private juce::Timer {
public:
    ResourceMeter(juce::AudioAppComponent& app) : audioApp(app) {
        // 4 Hz es el estándar de la industria para medidores de UI para no consumir recursos
        startTimerHz(4); 
    }
    
    ~ResourceMeter() override { 
        stopTimer(); 
    }

    void paint(juce::Graphics& g) override {
        // Fondo oscuro del medidor
        g.fillAll(juce::Colour(20, 22, 25));
        g.setColour(juce::Colours::grey.withAlpha(0.3f));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.0f, 1.0f);

        // Lógica de colores de advertencia
        juce::Colour meterColor = juce::Colours::limegreen;
        if (dspLoad > 70.0) meterColor = juce::Colours::orange;
        if (dspLoad > 85.0) meterColor = juce::Colours::red;

        g.setColour(meterColor);
        g.setFont(14.0f);
        g.drawText("DSP: " + juce::String(dspLoad, 1) + "%", getLocalBounds(), juce::Justification::centred, false);
    }

    void timerCallback() override {
        // getCpuUsage() lee la carga de procesamiento del hilo de audio (0.0 a 1.0)
        dspLoad = audioApp.deviceManager.getCpuUsage() * 100.0;
        repaint();
    }

private:
    juce::AudioAppComponent& audioApp;
    double dspLoad = 0.0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResourceMeter)
};