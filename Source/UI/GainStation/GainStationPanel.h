#pragma once
#include <JuceHeader.h>
#include "../../Tracks/Track.h"
#include "../Knobs/FloatingValueSlider.h"
#include "GainStationMeter.h"

class GainStationPanel : public juce::Component {
public:
    GainStationPanel() {
        addAndMakeVisible(meter);

        addAndMakeVisible(preGainKnob);
        preGainKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        preGainKnob.setRange(0.0, 2.0, 0.01);
        preGainKnob.setValue(1.0, juce::dontSendNotification);
        preGainKnob.setTooltip("Pre-Gain (Input Trim): Ajusta la entrada antes de los FX.");
        preGainKnob.onValueChange = [this] {
            if (activeTrack) activeTrack->setPreGain((float)preGainKnob.getValue());
            };

        addAndMakeVisible(postGainKnob);
        postGainKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        postGainKnob.setRange(0.0, 2.0, 0.01);
        postGainKnob.setValue(1.0, juce::dontSendNotification);
        postGainKnob.setTooltip("Post-Gain (Output Trim): Compensa la ganancia tras los FX.");
        postGainKnob.onValueChange = [this] {
            if (activeTrack) activeTrack->setPostGain((float)postGainKnob.getValue());
            };

        addAndMakeVisible(phaseBtn);
        phaseBtn.setButtonText(juce::CharPointer_UTF8("\xc3\x98")); // Símbolo de fase
        phaseBtn.setClickingTogglesState(true);
        phaseBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker(0.2f));
        phaseBtn.setTooltip("Invertir Polaridad (Fase)");
        phaseBtn.onClick = [this] {
            if (activeTrack) activeTrack->isPhaseInverted = phaseBtn.getToggleState();
            };

        addAndMakeVisible(monoBtn);
        monoBtn.setButtonText("MONO");
        monoBtn.setClickingTogglesState(true);
        monoBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange.darker(0.2f));
        monoBtn.setTooltip("Sumar a Mono (L+R)");
        monoBtn.onClick = [this] {
            if (activeTrack) activeTrack->isMonoActive = monoBtn.getToggleState();
            };

        addAndMakeVisible(matchBtn);
        matchBtn.setButtonText("MATCH");
        matchBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen.darker(0.5f));
        matchBtn.setTooltip("Iguala el volumen de salida con el de entrada automáticamente.");
        matchBtn.onClick = [this] { performGainMatch(); };
    }

    void setTrack(Track* t) {
        activeTrack = t;
        meter.setTrack(t);
        if (activeTrack) {
            preGainKnob.setValue(activeTrack->getPreGain(), juce::dontSendNotification);
            postGainKnob.setValue(activeTrack->getPostGain(), juce::dontSendNotification);
            phaseBtn.setToggleState(activeTrack->isPhaseInverted, juce::dontSendNotification);
            monoBtn.setToggleState(activeTrack->isMonoActive, juce::dontSendNotification);
        }
        repaint();
    }

    void paint(juce::Graphics& g) override {
        // Textos descriptivos debajo de los knobs dinámicos
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::Font("Sans-Serif", 11.0f, juce::Font::bold));

        if (preGainKnob.isVisible()) {
            auto preBounds = preGainKnob.getBounds();
            g.drawText("PRE", preBounds.getX(), preBounds.getBottom(), preBounds.getWidth(), 14, juce::Justification::centredTop);
        }

        if (postGainKnob.isVisible()) {
            auto postBounds = postGainKnob.getBounds();
            g.drawText("POST", postBounds.getX(), postBounds.getBottom(), postBounds.getWidth(), 14, juce::Justification::centredTop);
        }
    }

    void resized() override {
        auto area = getLocalBounds().reduced(6);

        // 1. EL MEDIDOR (Ocupa la mitad izquierda de todo el alto disponible)
        meter.setBounds(area.removeFromLeft(getWidth() / 2));

        area.removeFromLeft(8); // Espaciador vertical central entre el medidor y los controles

        // 2. LOS CONTROLES (Ocupan la mitad derecha, alineados con los medidores)
        auto rightArea = area;
        int halfH = rightArea.getHeight() / 2;

        // --- SECCIÓN PRE-FX (Alineada con el medidor superior) ---
        auto preArea = rightArea.removeFromTop(halfH);

        // Knob PRE ocupa la mayor parte arriba
        auto preKnobArea = preArea.removeFromTop((int)(preArea.getHeight() * 0.60f));
        preGainKnob.setBounds(preKnobArea.reduced(8, 2));

        preArea.removeFromTop(14); // Espacio reservado para el texto "PRE" dibujado en paint()

        // Botón de Fase centrado abajo del knob PRE
        phaseBtn.setBounds(preArea.removeFromTop(22).reduced(12, 0));


        // --- SECCIÓN POST-FX (Alineada con el medidor inferior) ---
        auto postArea = rightArea;

        // Knob POST ocupa la mayor parte arriba
        auto postKnobArea = postArea.removeFromTop((int)(postArea.getHeight() * 0.55f));
        postGainKnob.setBounds(postKnobArea.reduced(8, 2));

        postArea.removeFromTop(14); // Espacio reservado para el texto "POST" dibujado en paint()

        // Botones Mono y Match lado a lado en la parte inferior
        auto postBtnsArea = postArea.removeFromBottom(24);
        monoBtn.setBounds(postBtnsArea.removeFromLeft(postBtnsArea.getWidth() / 2).reduced(2, 0));
        matchBtn.setBounds(postBtnsArea.reduced(2, 0));
    }

private:
    Track* activeTrack = nullptr;
    GainStationMeter meter;
    FloatingValueSlider preGainKnob, postGainKnob;
    juce::TextButton phaseBtn, monoBtn, matchBtn;

    void performGainMatch() {
        if (!activeTrack) return;
        float inLufs = activeTrack->preLoudness.getShortTerm();
        float outLufs = activeTrack->postLoudness.getShortTerm();

        if (inLufs > -60.0f && outLufs > -60.0f) {
            float diffDb = inLufs - outLufs;
            float currentGainDb = juce::Decibels::gainToDecibels((float)postGainKnob.getValue());
            float newGain = juce::Decibels::decibelsToGain(currentGainDb + diffDb);

            if (newGain > 2.0f) newGain = 2.0f;
            if (newGain < 0.0f) newGain = 0.0f;

            postGainKnob.setValue(newGain, juce::sendNotification);
        }
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainStationPanel)
};