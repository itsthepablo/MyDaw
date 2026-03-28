#pragma once
#include <JuceHeader.h>
#include "../Engine/PDCManager.h" // Importamos el cerebro PDC

#if JUCE_WINDOWS
#define NOMINMAX  
#include <windows.h>
#include <psapi.h>
#endif

class ResourceMeter : public juce::Component, private juce::Timer {
public:
    ResourceMeter(juce::AudioAppComponent& app) : audioApp(app) {
        startTimerHz(4);
    }

    ~ResourceMeter() override {
        stopTimer();
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(20, 22, 25));
        g.setColour(juce::Colours::grey.withAlpha(0.3f));
        g.drawRoundedRectangle(getLocalBounds().toFloat(), 4.0f, 1.0f);

        juce::Colour meterColor = juce::Colours::limegreen;
        if (dspLoad > 70.0) meterColor = juce::Colours::orange;
        if (dspLoad > 85.0) meterColor = juce::Colours::red;

        g.setFont(11.0f);

        auto area = getLocalBounds();
        int colWidth = getWidth() / 3; // Dividimos en 3 paneles técnicos

        // --- COLUMNA 1: Sistema ---
        auto col1 = area.removeFromLeft(colWidth);
        g.setColour(meterColor);
        g.drawText("DSP: " + juce::String(dspLoad, 1) + "%",
            col1.removeFromTop(getHeight() / 2), juce::Justification::centred, false);
        g.setColour(juce::Colours::cyan.darker(0.1f));
        g.drawText("RAM: " + juce::String(ramLoadMB) + "MB",
            col1, juce::Justification::centred, false);

        // --- COLUMNA 2: Hardware ---
        auto col2 = area.removeFromLeft(colWidth);
        g.setColour(juce::Colours::lightgrey);
        g.drawText("BUF: " + juce::String(bufferSize),
            col2.removeFromTop(getHeight() / 2), juce::Justification::centred, false);
        g.setColour(juce::Colours::yellow);
        g.drawText("LAT: " + juce::String(latencyMs, 1) + "ms",
            col2, juce::Justification::centred, false);

        // --- COLUMNA 3: PDC (NUEVO) ---
        auto col3 = area;
        g.setColour(juce::Colours::orange);
        g.drawText("PDC SMP",
            col3.removeFromTop(getHeight() / 2), juce::Justification::centred, false);
        g.setColour(juce::Colours::white);
        // Leemos la latencia global matemáticamente
        g.drawText(juce::String(PDCManager::currentGlobalLatency),
            col3, juce::Justification::centred, false);
    }

    void timerCallback() override {
        dspLoad = audioApp.deviceManager.getCpuUsage() * 100.0;

#if JUCE_WINDOWS
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            ramLoadMB = (int)(pmc.PrivateUsage / (1024 * 1024));
        }
#else
        ramLoadMB = 0;
#endif

        if (auto* device = audioApp.deviceManager.getCurrentAudioDevice()) {
            bufferSize = device->getCurrentBufferSizeSamples();
            sampleRate = device->getCurrentSampleRate();

            if (sampleRate > 0.0) {
                latencyMs = (static_cast<double>(bufferSize) / sampleRate) * 1000.0;
            }
            else {
                latencyMs = 0.0;
            }
        }
        else {
            bufferSize = 0;
            sampleRate = 0.0;
            latencyMs = 0.0;
        }

        repaint();
    }

private:
    juce::AudioAppComponent& audioApp;

    double dspLoad = 0.0;
    int ramLoadMB = 0;

    int bufferSize = 0;
    double sampleRate = 0.0;
    double latencyMs = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResourceMeter)
};