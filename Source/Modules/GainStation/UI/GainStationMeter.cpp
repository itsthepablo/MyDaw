#include "GainStationMeter.h"

GainStationMeter::GainStationMeter() {
    startTimerHz(30);
}

GainStationMeter::~GainStationMeter() {
    stopTimer();
}

void GainStationMeter::setBridge(GainStationBridge* b) {
    activeBridge = b;
}

void GainStationMeter::timerCallback() {
    if (activeBridge && isShowing()) {
        repaint();
    }
}

void GainStationMeter::paint(juce::Graphics& g) {
    auto area = getLocalBounds();
    
    // Fondo oscuro profesional
    g.setColour(juce::Colour(15, 18, 20));
    g.fillRoundedRectangle(area.toFloat(), 4.0f);

    if (!activeBridge) return;

    // --- DIBUJAR VALORES NUMÉRICOS (Estilo Youlean/Insight) ---
    auto drawValue = [&](juce::Rectangle<float> box, juce::String label, float value, juce::Colour col) {
        // Fondo de la caja de valor
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRoundedRectangle(box.reduced(2), 2.0f);

        // Etiqueta (Pre/Post)
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(juce::Font("Sans-Serif", 10.0f, juce::Font::plain));
        g.drawText(label, box.withTrimmedBottom(box.getHeight() * 0.7f), juce::Justification::centred);

        // Valor Numérico
        juce::String valStr = (value <= -70.0f) ? "-inf" : juce::String(value, 1);
        g.setColour(col);
        g.setFont(juce::Font("Consolas", 16.0f, juce::Font::bold)); // Fuente digital
        g.drawText(valStr, box.withTrimmedTop(12), juce::Justification::centred);
        
        // Unidad (LUFS)
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.setFont(juce::Font(9.0f));
        g.drawText("LUFS S", box.withY(box.getBottom() - 12).withHeight(10), juce::Justification::centred);
    };

    auto h = (float)getHeight() / 2.0f;
    
    // Medidor PRE (Arriba)
    drawValue(area.removeFromTop(h).toFloat(), "INPUT", 
              activeBridge->getInputLUFS(), 
              juce::Colours::cyan.withAlpha(0.9f));
    
    // Medidor POST (Abajo)
    drawValue(area.toFloat(), "OUTPUT", 
              activeBridge->getOutputLUFS(), 
              juce::Colours::orange.withAlpha(0.9f));
}
