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

        auto debugArea = getLocalBounds().removeFromBottom(14);
        auto area = getLocalBounds().withBottom(debugArea.getY()).reduced(4, 0);

        juce::Colour meterColor = juce::Colours::limegreen;
        if (dspLoad > 70.0) meterColor = juce::Colours::orange;
        if (dspLoad > 85.0) meterColor = juce::Colours::red;

        g.setFont(12.0f);

        int colWidth = area.getWidth() / 5; // Reorganizamos de forma 100% horizontal en 5 columnas

        // --- CPU ---
        auto c1 = area.removeFromLeft(colWidth);
        g.setColour(meterColor);
        g.drawText("CPU: " + juce::String(dspLoad, 1) + "%", c1, juce::Justification::centred, false);

        // --- RAM ---
        auto c2 = area.removeFromLeft(colWidth);
        g.setColour(juce::Colours::cyan.darker(0.1f));
        g.drawText("RAM: " + juce::String(ramLoadMB) + "M", c2, juce::Justification::centred, false);

        // --- Buffer Size ---
        auto c3 = area.removeFromLeft(colWidth);
        g.setColour(juce::Colours::lightgrey);
        g.drawText("BUF: " + juce::String(bufferSize), c3, juce::Justification::centred, false);

        // --- Latencia ---
        auto c4 = area.removeFromLeft(colWidth);
        g.setColour(juce::Colours::yellow);
        g.drawText("LAT: " + juce::String(latencyMs, 1) + "ms", c4, juce::Justification::centred, false);

        // --- PDC Global ---
        auto c5 = area;
        g.setColour(juce::Colours::orange);
        g.drawText("PDC: " + juce::String(PDCManager::currentGlobalLatency), c5, juce::Justification::centred, false);

        // --- DEBUG PANEL ---
        g.setColour(juce::Colours::white);
        g.setFont(10.0f);
        juce::String debugInfo = "Trks: " + juce::String(PDCManager::dbgTracks.load()) +
                                 " | Play: " + juce::String(PDCManager::dbgPlaying.load()) +
                                 " | Clips: " + juce::String(PDCManager::dbgClips.load()) +
                                 " | Wrtn: " + juce::String(PDCManager::dbgSamplesWritten.load()) +
                                 " | Adds: " + juce::String(PDCManager::dbgAddCount.load());
        g.drawText(debugInfo, debugArea, juce::Justification::centred, false);
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