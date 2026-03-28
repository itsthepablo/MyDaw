#pragma once
#include <JuceHeader.h>

// --- EL ESCUDO CONTRA LA BASURA DE WINDOWS ---
#if JUCE_WINDOWS
#define NOMINMAX  
#include <windows.h>
#include <psapi.h>
#endif

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

        // Lógica de colores de advertencia para DSP
        juce::Colour meterColor = juce::Colours::limegreen;
        if (dspLoad > 70.0) meterColor = juce::Colours::orange;
        if (dspLoad > 85.0) meterColor = juce::Colours::red;

        g.setFont(11.0f); // Fuente optimizada para panel de diagnóstico a 4 variables

        auto area = getLocalBounds();
        auto topRow = area.removeFromTop(getHeight() / 2);
        auto bottomRow = area;

        int colWidth = getWidth() / 2; // Dividimos en 2 columnas

        // --- COLUMNA IZQUIERDA (Carga del Sistema) ---
        g.setColour(meterColor);
        g.drawText("DSP: " + juce::String(dspLoad, 1) + "%",
            topRow.removeFromLeft(colWidth), juce::Justification::centred, false);

        g.setColour(juce::Colours::cyan.darker(0.1f));
        g.drawText("RAM: " + juce::String(ramLoadMB) + "MB",
            bottomRow.removeFromLeft(colWidth), juce::Justification::centred, false);

        // --- COLUMNA DERECHA (Estado del Motor de Audio) ---
        g.setColour(juce::Colours::lightgrey);
        g.drawText("BUF: " + juce::String(bufferSize),
            topRow, juce::Justification::centred, false);

        g.setColour(juce::Colours::yellow);
        g.drawText("LAT: " + juce::String(latencyMs, 1) + "ms",
            bottomRow, juce::Justification::centred, false);
    }

    void timerCallback() override {
        // 1. Leer DSP
        dspLoad = audioApp.deviceManager.getCpuUsage() * 100.0;

        // 2. Leer consumo exacto de RAM del DAW en Windows
#if JUCE_WINDOWS
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            ramLoadMB = (int)(pmc.PrivateUsage / (1024 * 1024));
        }
#else
        ramLoadMB = 0; // Fallback por si compilas en Mac/Linux después
#endif

        // 3. Leer estado del hardware de Audio (Búfer y Latencia Real)
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