#pragma once
#include <JuceHeader.h>
#include "../Engine/Routing/PDCManager.h" // Importamos el cerebro PDC

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

        auto bounds = getLocalBounds().reduced(4);
        auto topRow = bounds.removeFromTop(bounds.getHeight() / 2);
        auto botRow = bounds;

        juce::Colour meterColor = juce::Colours::limegreen;
        if (dspLoad > 70.0) meterColor = juce::Colours::orange;
        if (dspLoad > 85.0) meterColor = juce::Colours::red;

        g.setFont(12.0f);

        // --- FILA SUPERIOR (CPU, RAM, BUF) ---
        int topCols = 3;
        int wTop = topRow.getWidth() / topCols;

        auto cCPU = topRow.removeFromLeft(wTop);
        g.setColour(meterColor);
        g.drawText("CPU: " + juce::String((int)std::round(dspLoad)) + "%", cCPU, juce::Justification::centred, false);

        auto cRAM = topRow.removeFromLeft(wTop);
        g.setColour(juce::Colours::cyan.darker(0.1f));
        g.drawText("RAM: " + juce::String(ramLoadMB) + "MB", cRAM, juce::Justification::centred, false);

        auto cBUF = topRow;
        g.setColour(juce::Colours::lightgrey);
        g.drawText("BUF: " + juce::String(bufferSize), cBUF, juce::Justification::centred, false);

        // --- FILA INFERIOR (LAT, PDC) ---
        int botCols = 2;
        int wBot = botRow.getWidth() / botCols;

        auto cLAT = botRow.removeFromLeft(wBot);
        g.setColour(juce::Colours::yellow);
        g.drawText("LAT: " + juce::String(latencyMs, 1) + "ms", cLAT, juce::Justification::centred, false);

        auto cPDC = botRow;
        g.setColour(juce::Colours::orange);
        g.drawText("PDC: " + juce::String(PDCManager::currentGlobalLatency), cPDC, juce::Justification::centred, false);
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