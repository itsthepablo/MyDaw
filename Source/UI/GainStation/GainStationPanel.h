#pragma once
#include <JuceHeader.h>
#include "../../Tracks/Track.h"
#include "../../Mixer/Bridges/MixerParameterBridge.h"
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
        phaseBtn.setButtonText(juce::CharPointer_UTF8("\xc3\x98"));
        phaseBtn.setClickingTogglesState(true);
        phaseBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker(0.2f));
        phaseBtn.setTooltip("Invertir Polaridad (Fase)");
        phaseBtn.onClick = [this] {
            if (activeTrack) MixerParameterBridge::setPhaseInverted(activeTrack, phaseBtn.getToggleState());
            };

        addAndMakeVisible(monoBtn);
        monoBtn.setButtonText("MONO");
        monoBtn.setClickingTogglesState(true);
        monoBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange.darker(0.2f));
        monoBtn.setTooltip("Sumar a Mono (L+R)");
        monoBtn.onClick = [this] {
            if (activeTrack) MixerParameterBridge::setMonoActive(activeTrack, monoBtn.getToggleState());
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
            phaseBtn.setToggleState(MixerParameterBridge::isPhaseInverted(activeTrack), juce::dontSendNotification);
            monoBtn.setToggleState(MixerParameterBridge::isMonoActive(activeTrack), juce::dontSendNotification);
        }
        repaint();
    }

    void paint(juce::Graphics& g) override {
        // --- CORRECCIÓN CRÍTICA DE INTEGRACIÓN ---
        // Ahora comparte el mismo color exacto del panel de track info (30, 33, 36).
        // Se ha borrado el "drawRect" que lo hacía parecer un slot de plugin independiente.
        g.fillAll(juce::Colour(30, 33, 36));

        // Líneas organizativas internas
        g.setColour(juce::Colour(50, 53, 56));
        g.drawHorizontalLine(getHeight() / 2, 10.0f, (float)getWidth() - 10.0f);
        g.drawVerticalLine(getWidth() / 2, 10.0f, (float)getHeight() - 10.0f);

        // Texto descriptivo inferior
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(juce::Font("Sans-Serif", 11.0f, juce::Font::bold));

        if (preGainKnob.isVisible()) {
            auto preBounds = preGainKnob.getBounds();
            g.drawText("PRE", preBounds.getX(), preBounds.getBottom() + 2, preBounds.getWidth(), 14, juce::Justification::centredTop);
        }

        if (postGainKnob.isVisible()) {
            auto postBounds = postGainKnob.getBounds();
            g.drawText("POST", postBounds.getX(), postBounds.getBottom() + 2, postBounds.getWidth(), 14, juce::Justification::centredTop);
        }
    }

    void resized() override {
        auto area = getLocalBounds().reduced(6);

        // El medidor ocupa la mitad izquierda
        meter.setBounds(area.removeFromLeft(getWidth() / 2 - 6));

        area.removeFromLeft(6); // Espaciador vertical central

        // Los controles ocupan la mitad derecha
        auto rightArea = area;
        int halfH = rightArea.getHeight() / 2;

        // --- SECCIÓN PRE-FX ---
        auto preArea = rightArea.removeFromTop(halfH);

        auto preKnobArea = preArea.removeFromTop((int)(preArea.getHeight() * 0.55f));
        preGainKnob.setBounds(preKnobArea.reduced(6, 2));

        preArea.removeFromTop(18); // Espacio obligatorio para el texto

        phaseBtn.setBounds(preArea.removeFromTop(20).reduced(12, 0));

        // --- SECCIÓN POST-FX ---
        auto postArea = rightArea;

        auto postKnobArea = postArea.removeFromTop((int)(postArea.getHeight() * 0.55f));
        postGainKnob.setBounds(postKnobArea.reduced(6, 2));

        postArea.removeFromTop(18); // Espacio obligatorio para el texto

        auto postBtnsArea = postArea.removeFromBottom(22);
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