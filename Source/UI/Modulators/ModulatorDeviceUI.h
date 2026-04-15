#pragma once
#include <JuceHeader.h>
#include "../../../Engine/Modulation/GridModulator.h"
#include "../../../Engine/Core/TransportState.h"
#include "ModulatorKnob.h"

/**
 * ModulatorDeviceUI: Interfaz de Grado Profesional "Master Sync".
 * Implementa visualización Serum-style con sincronía frame-perfect.
 */
class ModulatorDeviceUI : public juce::Component, public juce::Timer {
public:
    ModulatorDeviceUI(GridModulator& m, TransportState* ts) : modulator(m),
                                          transportState(ts),
                                          rateKnob("RATE"), 
                                          depthKnob("DEPTH")
    {
        addAndMakeVisible(nameLabel);
        nameLabel.setText("LFO PRO", juce::dontSendNotification);
        nameLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        // --- RATE ---
        addAndMakeVisible(rateKnob);
        rateKnob.isRate = true;
        rateKnob.setRange(0, 23, 1);
        rateKnob.setValue(getIndexForRate(modulator.rate.load()), juce::dontSendNotification);
        rateKnob.onValueChange = [this] { 
            int idx = juce::jlimit(0, 23, (int)std::round(rateKnob.getValue()));
            modulator.rate.store(getRateForIndex(idx)); 
        };

        // --- DEPTH ---
        addAndMakeVisible(depthKnob);
        depthKnob.setRange(0.0, 1.0);
        depthKnob.setValue(modulator.depth.load());
        depthKnob.onValueChange = [this] { modulator.depth.store((float)depthKnob.getValue()); };

        // --- SHAPE ---
        addAndMakeVisible(shapeCombo);
        shapeCombo.addItem("Sine", 1);
        shapeCombo.addItem("Saw", 2);
        shapeCombo.addItem("Square", 3);
        shapeCombo.addItem("Triangle", 4);
        shapeCombo.addItem("Random", 5);
        shapeCombo.setSelectedId(modulator.shape.load() + 1);
        shapeCombo.onChange = [this] { modulator.shape.store(shapeCombo.getSelectedId() - 1); };

        // --- TARGETS ---
        addAndMakeVisible(targetBtn);
        targetBtn.setButtonText("MOD: 0 Targets");
        targetBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        targetBtn.onClick = [this] { showTargetMenu(); };

        addAndMakeVisible(learnBtn);
        learnBtn.setButtonText("LEARN");
        learnBtn.setClickingTogglesState(true);
        learnBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
        learnBtn.onClick = [this] { 
            if (learnBtn.getToggleState()) GridModulator::pendingModulator = &modulator;
            else if (GridModulator::pendingModulator == &modulator) GridModulator::pendingModulator = nullptr;
        };

        startTimerHz(60); 
    }

    void paint(juce::Graphics& g) override {
        auto area = getLocalBounds().toFloat();
        
        // --- Fondo y Borde Principal ---
        g.setColour(juce::Colour(25, 27, 30));
        g.fillRoundedRectangle(area, 6.0f);
        g.setColour(juce::Colour(60, 65, 70));
        g.drawRoundedRectangle(area.reduced(0.5f), 6.0f, 1.0f);

        // --- Area de Visualización de Onda ---
        auto waveArea = getLocalBounds().reduced(150, 25).withTrimmedLeft(180).toFloat();
        g.setColour(juce::Colour(15, 17, 20));
        g.fillRoundedRectangle(waveArea, 4.0f);
        g.setColour(juce::Colour(40, 42, 45));
        g.drawRoundedRectangle(waveArea, 4.0f, 1.0f);

        // --- Dibujo de la Onda Estática (Fondo) ---
        juce::Path p;
        float w = waveArea.getWidth();
        float h = waveArea.getHeight();
        float midY = waveArea.getCentreY();
        float d = modulator.depth.load();

        if (modulator.shape.load() != 4) { // No random para el fondo estático por ahora
            for (float x = 0; x <= w; x += 1.0f) {
                double ph = (double)x / w;
                float val = modulator.getRawShapeValue(ph) * d;
                float y = midY - (val * h * 0.40f);
                
                if (x == 0) p.startNewSubPath(waveArea.getX() + x, y);
                else p.lineTo(waveArea.getX() + x, y);
            }
            // Dibujar línea de fondo sutil
            g.setColour(juce::Colour(100, 180, 255).withAlpha(0.2f));
            g.strokePath(p, juce::PathStrokeType(1.5f));
            
            // Dibujar relleno gradiente
            juce::Path fillPath = p;
            fillPath.lineTo(waveArea.getRight(), waveArea.getBottom());
            fillPath.lineTo(waveArea.getX(), waveArea.getBottom());
            fillPath.closeSubPath();
            
            juce::ColourGradient grad(juce::Colour(0, 150, 255).withAlpha(0.15f), 0, waveArea.getY(),
                                     juce::Colour(0, 50, 100).withAlpha(0.0f), 0, waveArea.getBottom(), false);
            g.setGradientFill(grad);
            g.fillPath(fillPath);
        }

        // --- INDICADOR DINÁMICO (SYNC PRO) ---
        double absolutePixels = 0.0;
        double interpPh = 0.0;

        if (transportState && transportState->isPlaying.load()) {
            double headPx = (double)transportState->currentAudioPlayhead.load();
            double lastMs = transportState->lastPlayheadUpdateMs.load();
            double nowMs = juce::Time::getMillisecondCounterHiRes();
            float currentBpm = transportState->bpm.load();
            
            // Interpolación Pro: Predecir posición actual basada en tiempo transcurrido
            double elapsedS = (nowMs - lastMs) * 0.001;
            double pixelsPerSec = (currentBpm / 60.0) * GridModulator::PIXELS_PER_BEAT;
            absolutePixels = headPx + (elapsedS * pixelsPerSec);
            interpPh = modulator.getVisualPhase(absolutePixels);
        } else {
            double timeInSeconds = juce::Time::getMillisecondCounterHiRes() * 0.001;
            float currentBpm = transportState ? transportState->bpm.load() : 120.0f;
            absolutePixels = timeInSeconds * (currentBpm / 60.0) * GridModulator::PIXELS_PER_BEAT;
            interpPh = modulator.getVisualPhase(absolutePixels);
        }

        // --- Dibujar Punto de Playhead ---
        float dotX = waveArea.getX() + (float)interpPh * w;
        float dotVal = modulator.getVisualValueAt(absolutePixels);
        float dotY = midY - (dotVal * h * 0.40f);

        // Glow
        g.setColour(juce::Colour(0, 200, 255).withAlpha(0.3f));
        g.fillEllipse(dotX - 7, dotY - 7, 14, 14);
        
        // Punto núcleo
        g.setColour(juce::Colours::white);
        g.fillEllipse(dotX - 4, dotY - 4, 8, 8);
        g.setColour(juce::Colour(0, 200, 255));
        g.drawEllipse(dotX - 4, dotY - 4, 8, 8, 1.5f);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(15);
        auto left = area.removeFromLeft(160);
        nameLabel.setBounds(left.removeFromTop(20));
        auto btnRow = left.removeFromTop(30);
        learnBtn.setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2).reduced(2));
        targetBtn.setBounds(btnRow.reduced(2));
        shapeCombo.setBounds(left.removeFromTop(25).reduced(2, 5));

        auto right = area.removeFromRight(180);
        int knobW = right.getWidth() / 2;
        rateKnob.setBounds(right.removeFromLeft(knobW));
        depthKnob.setBounds(right);
    }

    void timerCallback() override {
        updateTargetButtonText();
        if (GridModulator::pendingModulator != &modulator && learnBtn.getToggleState()) {
            learnBtn.setToggleState(false, juce::dontSendNotification);
        }
        repaint(); 
    }

private:
    float getRateForIndex(int idx) {
        static const float rates[] = { 
            32.0f, 16.0f, 8.0f, 6.0f, 4.0f, 3.0f, 8.0f/3.0f, 2.0f, 
            1.5f, 4.0f/3.0f, 1.0f, 0.75f, 2.0f/3.0f, 0.5f, 0.375f, 
            1.0f/3.0f, 0.25f, 0.1875f, 1.0f/6.0f, 0.125f, 1.0f/12.0f, 
            0.0625f, 1.0f/24.0f, 0.03125f 
        };
        return rates[juce::jlimit(0, 23, (int)idx)];
    }

    float getIndexForRate(float r) {
        float minRegion = 999.0f;
        int bestIdx = 0;
        for (int i = 0; i < 24; ++i) {
            float diff = std::abs(r - getRateForIndex(i));
            if (diff < minRegion) {
                minRegion = diff;
                bestIdx = i;
            }
        }
        return (float)bestIdx;
    }

    void updateTargetButtonText() {
        juce::ScopedLock sl(modulator.targetsLock);
        int count = modulator.targets.size();
        targetBtn.setButtonText("MOD: " + juce::String(count) + (count == 1 ? " Target" : " Targets"));
    }

    void showTargetMenu() {
        juce::PopupMenu m;
        m.addItem(1, "Plugin Parameter (LEARN)");
        m.addSeparator();
        m.addItem(99, "CLEAR ALL TARGETS");
        m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) {
            if (r == 99) { modulator.clearTargets(); }
        });
    }

    TransportState* transportState = nullptr;
    GridModulator& modulator;
    juce::Label nameLabel;
    ModulatorKnob rateKnob, depthKnob;
    juce::ComboBox shapeCombo;
    juce::TextButton targetBtn, learnBtn;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorDeviceUI)
};
