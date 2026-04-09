#pragma once
#include <JuceHeader.h>
#include "../../../Tracks/Track.h"

/**
 * LoudnessMeter — Rediseño Premium v2
 * Monitor de LUFS ultra-legible y reactivo.
 */
class LoudnessMeter : public juce::Component, private juce::Timer
{
public:
    LoudnessMeter(Track& t) : track(t)
    {
        startTimerHz(22); // Máxima fluidez visual
    }

    ~LoudnessMeter() override { stopTimer(); }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds().toFloat();
        
        // 1. FONDO OSCURO ABSOLUTO (Estilo OLED)
        g.setColour(juce::Colour(10, 10, 12));
        g.fillRoundedRectangle(area, 4.0f);
        
        // 2. BORDE DE ACENTO (Naranja Vibrante para Loudness)
        g.setColour(track.getColor().withAlpha(0.6f));
        g.drawRoundedRectangle(area.reduced(0.5f), 4.0f, 1.2f);

        auto content = area.reduced(6.0f, 4.0f);
        
        // 3. ETIQUETA SUPERIOR (Sutil)
        g.setColour(juce::Colours::grey.withAlpha(0.5f));
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText("LUFS SHORT-TERM", content.removeFromTop(12.0f), juce::Justification::centred);

        // 4. VALOR PRINCIPAL (El corazón del diseño)
        float st = track.postLoudness.getShortTerm();
        juce::String stStr = (st <= -70.0f) ? "-inf" : juce::String(st, 1);
        
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(38.0f, juce::Font::bold)); // Tamaño máximo para 80px disponibles
        g.drawText(stStr, content.removeFromTop(40.0f), juce::Justification::centred);

        // 5. MÉTRICAS INFERIORES (Distribución en columnas)
        auto bottomRow = content.removeFromBottom(22.0f);
        auto colWidth = bottomRow.getWidth() / 3.0f;

        auto drawMetric = [&](juce::Rectangle<float> r, juce::String label, juce::String val, juce::Colour valCol) {
            auto labelArea = r.removeFromTop(8.0f);
            g.setColour(juce::Colours::grey.withAlpha(0.6f));
            g.setFont(juce::Font(8.0f, juce::Font::plain));
            g.drawText(label, labelArea, juce::Justification::centred);
            
            g.setColour(valCol);
            g.setFont(juce::Font(12.0f, juce::Font::bold));
            g.drawText(val, r, juce::Justification::centred);
        };

        // Range (LRA)
        float lra = track.postLoudness.getRange();
        drawMetric(bottomRow.removeFromLeft(colWidth), "RANGE", juce::String(lra, 1), juce::Colours::orange.withAlpha(0.9f));

        // True Peak (dBTP)
        float tp = track.postLoudness.getTruePeak();
        float tpDb = juce::Decibels::gainToDecibels(tp, -100.0f);
        juce::String tpStr = (tpDb <= -100.0f) ? "-inf" : juce::String(tpDb, 1);
        juce::Colour tpCol = (tpDb > -0.1f) ? juce::Colours::red : juce::Colours::cyan.withAlpha(0.8f);
        drawMetric(bottomRow.removeFromLeft(colWidth), "dBTP", tpStr, tpCol);

        // Integrated (Long)
        float lng = track.postLoudness.getIntegrated();
        juce::String lngStr = (lng <= -70.0f) ? "-inf" : juce::String(lng, 1);
        drawMetric(bottomRow, "LONG", lngStr, juce::Colours::magenta.withAlpha(0.8f));
    }

private:
    void timerCallback() override
    {
        static int slowCount = 0;
        if (++slowCount >= 10) {
            track.postLoudness.calculateIntegratedAndLRA();
            slowCount = 0;
        }
        repaint();
    }

    Track& track;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoudnessMeter)
};
