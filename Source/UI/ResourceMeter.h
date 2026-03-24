#pragma once
#include <JuceHeader.h>

// --- EL ESCUDO CONTRA LA BASURA DE WINDOWS ---
#if JUCE_WINDOWS
#define NOMINMAX  // <- ESTA ES LA LÍNEA MÁGICA QUE ARREGLA TUS ERRORES
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

        g.setFont(12.0f); // Fuente un poco más pequeña para que quepan dos textos

        // 1. Dibujar texto de DSP en la mitad superior
        g.setColour(meterColor);
        g.drawText("DSP: " + juce::String(dspLoad, 1) + "%",
            getLocalBounds().removeFromTop(getHeight() / 2),
            juce::Justification::centred, false);

        // 2. Dibujar texto de RAM en la mitad inferior
        g.setColour(juce::Colours::cyan.darker(0.1f));
        g.drawText("RAM: " + juce::String(ramLoadMB) + " MB",
            getLocalBounds().removeFromBottom(getHeight() / 2),
            juce::Justification::centred, false);
    }

    void timerCallback() override {
        // Leer DSP
        dspLoad = audioApp.deviceManager.getCpuUsage() * 100.0;

        // Leer consumo exacto de RAM del DAW en Windows
#if JUCE_WINDOWS
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            // pmc.PrivateUsage nos da los bytes usados. Lo dividimos para obtener Megabytes
            ramLoadMB = (int)(pmc.PrivateUsage / (1024 * 1024));
        }
#else
        ramLoadMB = 0; // Fallback por si compilas en Mac/Linux después
#endif

        repaint();
    }

private:
    juce::AudioAppComponent& audioApp;
    double dspLoad = 0.0;
    int ramLoadMB = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ResourceMeter)
};