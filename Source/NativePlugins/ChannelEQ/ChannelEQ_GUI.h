#pragma once
#include <JuceHeader.h>
#include "ChannelEQ_DSP.h"

#include <cmath>

/**
 * @class ChannelEQ_Editor
 * Interfaz de usuario de Grado Profesional (Estilo Serum) para el Channel EQ.
 */
class ChannelEQ_Editor : public juce::Component, public juce::Timer {
public:
    ChannelEQ_Editor()
        : b1HP("HP", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
          b1Bell("Bell", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
          b1LS("LS", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
          b2LP("LP", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
          b2Bell("Bell", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey),
          b2HS("HS", juce::Colours::grey, juce::Colours::grey, juce::Colours::grey)
    {
        startTimerHz(60);

        // --- CONFIGURACIÓN DE KNOBS (Estilo Cyan) ---
        auto setupKnob = [this](juce::Slider& s, juce::Label& l, const juce::String& name, float min, float max, float def) {
            s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            s.setRange(min, max);
            s.setValue(def);
            s.setDoubleClickReturnValue(true, def);
            s.setPopupDisplayEnabled(true, true, this); // Burbuja flotante nativa
            s.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0, 220, 255));
            s.setColour(juce::Slider::thumbColourId, juce::Colours::white);
            s.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
            addAndMakeVisible(s);

            l.setText(name, juce::dontSendNotification);
            l.setJustificationType(juce::Justification::centred);
            l.setFont(juce::Font(10.0f, juce::Font::bold));
            l.setColour(juce::Label::textColourId, juce::Colours::grey);
            addAndMakeVisible(l);

            s.onValueChange = [this] { if (targetDSP) updateDSPFromUI(); };
        };

        setupKnob(lowFreq, lowFreqLabel, "FREQ", 20.0f, 20000.0f, 150.0f);
        lowFreq.setSkewFactorFromMidPoint(1000.0f);
        lowFreq.modTarget.type = ModTarget::EQ_B1_Freq;

        setupKnob(lowQ, lowQLabel, "Q", 0.1f, 10.0f, 0.707f);
        lowQ.modTarget.type = ModTarget::EQ_B1_Q;

        setupKnob(lowGain, lowGainLabel, "GAIN", -24.0f, 24.0f, 0.0f);
        lowGain.modTarget.type = ModTarget::EQ_B1_Gain;

        setupKnob(highFreq, highFreqLabel, "FREQ", 20.0f, 20000.0f, 5000.0f);
        highFreq.setSkewFactorFromMidPoint(1000.0f);
        highFreq.modTarget.type = ModTarget::EQ_B2_Freq;

        setupKnob(highQ, highQLabel, "Q", 0.1f, 10.0f, 0.707f);
        highQ.modTarget.type = ModTarget::EQ_B2_Q;

        setupKnob(highGain, highGainLabel, "GAIN", -24.0f, 24.0f, 0.0f);
        highGain.modTarget.type = ModTarget::EQ_B2_Gain;

        // --- BOTONES DE TIPO ---
        auto setupTypeBtn = [this](juce::ShapeButton& b, int id, bool isBand1) {
            b.setClickingTogglesState(true);
            b.setRadioGroupId(isBand1 ? 100 : 200);
            b.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            
            // DAR UNA FORMA PARA QUE SEA HIT-TESTABLE (Regla de oro: El usuario debe poder clickear)
            juce::Path p;
            p.addEllipse(0, 0, 10, 10);
            b.setShape(p, true, true, false);
            
            addAndMakeVisible(b);
            b.onClick = [this] { if (targetDSP) updateDSPFromUI(); };
        };

        setupTypeBtn(b1HP, 0, true);
        setupTypeBtn(b1Bell, 2, true);
        setupTypeBtn(b1LS, 1, true);
        b1Bell.setToggleState(true, juce::dontSendNotification);

        setupTypeBtn(b2LP, 4, false);
        setupTypeBtn(b2Bell, 2, false);
        setupTypeBtn(b2HS, 3, false);
        b2Bell.setToggleState(true, juce::dontSendNotification);

        // --- BYPASS & TRACK NAME ---
        bypassBtn.setButtonText("PWR");
        bypassBtn.setClickingTogglesState(true);
        bypassBtn.setToggleState(true, juce::dontSendNotification); // Default = ON (No bypass)
        bypassBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey.withAlpha(0.5f));
        bypassBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0, 220, 255).withAlpha(0.6f));
        bypassBtn.onClick = [this] { 
            if (targetDSP) {
                targetDSP->userBypass.store(!bypassBtn.getToggleState(), std::memory_order_relaxed);
            }
            repaint();
        };
        addAndMakeVisible(bypassBtn);

        trackNameLabel.setJustificationType(juce::Justification::centredLeft);
        trackNameLabel.setFont(juce::Font(14.0f, juce::Font::bold));
        trackNameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        trackNameLabel.setText("No Track", juce::dontSendNotification);
        addAndMakeVisible(trackNameLabel);
    }

    void setTrackName(const juce::String& name) {
        trackNameLabel.setText(name.toUpperCase(), juce::dontSendNotification);
    }

    void setDSP(ChannelEQ_DSP* dsp) {
        targetDSP = dsp;
        if (dsp) updateUIFromDSP();
    }

    void paint(juce::Graphics& g) override
    {
        // Fondo Oscuro Premium
        g.fillAll(juce::Colour(17, 20, 24));

        auto r = getLocalBounds().reduced(4);
        
        // 1. Dibujar Display Central (Limitado a su nueva área calculada por el layout horizontal)
        int knobSectionW = juce::jlimit(110, 150, static_cast<int>(r.getWidth() * 0.35f));
        int btnW = 24;
        auto displayArea = r.withTrimmedLeft(knobSectionW + btnW).withTrimmedRight(knobSectionW + btnW).reduced(2, 0);
        
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillRoundedRectangle(displayArea.toFloat(), 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(displayArea.toFloat(), 4.0f, 1.0f);

        // --- ESPECTRO VERDE (FFT) ---
        if (targetDSP) {
            g.saveState();
            g.reduceClipRegion(displayArea);
            
            auto& magnitudes = targetDSP->getMagnitudes();
            juce::Path fftPath;
            bool first = true;

            // Rango de visualización en dB: -100 dB a +12 dB (offset de 16 ya incluido en DSP)
            const float displayMinDB = -100.0f + 16.0f; 
            const float displayMaxDB = 10.0f + 16.0f;

            for (size_t i = 0; i < magnitudes.size(); ++i) {
                float x = displayArea.getX() + (float)i / magnitudes.size() * displayArea.getWidth();
                
                // Normalizar la magnitud en dB al área vertical con CLAMP de seguridad
                float db = magnitudes[i];
                float normY = juce::jmap(juce::jlimit(displayMinDB, displayMaxDB, db), displayMinDB, displayMaxDB, 0.0f, 1.0f);
                float y = displayArea.getBottom() - (normY * displayArea.getHeight() * 0.9f);
                
                if (first) { fftPath.startNewSubPath(x, y); first = false; }
                else fftPath.lineTo(x, y);
            }
            
            // --- DIBUJAR RELLENO (Degradado Vertical de Color a Transparente) ---
            juce::Path fillPath = fftPath;
            fillPath.lineTo((float)displayArea.getRight(), (float)displayArea.getBottom());
            fillPath.lineTo((float)displayArea.getX(), (float)displayArea.getBottom());
            fillPath.closeSubPath();

            juce::Colour emerald = juce::Colour(80, 200, 120);
            
            // Color sólido con opacidad alta para máxima claridad
            g.setColour(emerald.withAlpha(0.7f));
            g.fillPath(fillPath);
            
            // --- CURVA CIAN (IIR) ---
            drawResponseCurve(g, displayArea);
            g.restoreState();
        }

        // --- DIBUJAR ICONOS DE FILTRO ---
        drawFilterIcons(g);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced(2);

        int knobSectionW = juce::jlimit(110, 150, static_cast<int>(r.getWidth() * 0.35f));
        int btnW = 24; // Para los botones ShapeButton (HP, LP, etc)

        // --- BANDA 1 ---
        auto leftKnobs = r.removeFromLeft(knobSectionW);
        auto b1Area = r.removeFromLeft(btnW);
        
        // --- BANDA 2 ---
        auto rightKnobs = r.removeFromRight(knobSectionW);
        auto b2Area = r.removeFromRight(btnW);
        
        // --- PANEL DE INFORMACIÓN (Izquierda/Arriba) ---
        // Colocamos la info flotando arriba, usando el espacio que antes era del texto.
        auto infoArea = leftKnobs.removeFromTop(20);
        bypassBtn.setBounds(infoArea.removeFromLeft(40).reduced(2, 2));
        trackNameLabel.setBounds(infoArea);

        // Layout Banda 1 Knobs (Horizontal)
        int kSize = juce::jmin(42, knobSectionW / 3); // Tamaño proporcional dinámico del rotary
        // Dejamos un margen pequeño para que respiren.
        auto lkArea = leftKnobs.withTrimmedTop(5).withTrimmedBottom(5);
        
        auto placeKnob = [&](juce::Slider& s, juce::Label& l, juce::Rectangle<int> area) {
            s.setBounds(area.withSizeKeepingCentre(kSize, kSize));
            // Label de nombre (FREQ, Q, GAIN) va debajo
            l.setBounds(area.getX(), s.getBottom(), area.getWidth(), 15);
        };
        
        placeKnob(lowFreq, lowFreqLabel, lkArea.removeFromLeft(knobSectionW / 3));
        placeKnob(lowQ, lowQLabel, lkArea.removeFromLeft(knobSectionW / 3));
        placeKnob(lowGain, lowGainLabel, lkArea.removeFromLeft(knobSectionW / 3));

        // Botones Banda 1 (Vertical)
        int b1H = b1Area.getHeight() / 3;
        b1HP.setBounds(b1Area.removeFromTop(b1H).withSizeKeepingCentre(16, 16));
        b1Bell.setBounds(b1Area.removeFromTop(b1H).withSizeKeepingCentre(16, 16));
        b1LS.setBounds(b1Area.removeFromTop(b1H).withSizeKeepingCentre(16, 16));

        // Layout Banda 2 Knobs (Horizontal)
        auto rkArea = rightKnobs.withTrimmedTop(15).withTrimmedBottom(5);
        placeKnob(highFreq, highFreqLabel, rkArea.removeFromLeft(knobSectionW / 3));
        placeKnob(highQ, highQLabel, rkArea.removeFromLeft(knobSectionW / 3));
        placeKnob(highGain, highGainLabel, rkArea.removeFromLeft(knobSectionW / 3));

        // Botones Banda 2 (Vertical)
        int b2H = b2Area.getHeight() / 3;
        b2LP.setBounds(b2Area.removeFromTop(b2H).withSizeKeepingCentre(16, 16));
        b2Bell.setBounds(b2Area.removeFromTop(b2H).withSizeKeepingCentre(16, 16));
        b2HS.setBounds(b2Area.removeFromTop(b2H).withSizeKeepingCentre(16, 16));
    }

    void timerCallback() override { 
        if (targetDSP) {
            targetDSP->processAnalyzer();

            // --- SINCRONIZACIÓN VISUAL DE AUTOMATIZACIÓN (DESACOPLADA) ---
            // Solo movemos los knobs si hay modulación activa para cada parámetro específico
            auto& sync = targetDSP->visSync;

            auto updateVisual = [](juce::Slider& s, float target) {
                if (!s.isMouseButtonDown())
                    s.setValue(NativeVisualSync::smooth((float)s.getValue(), target, 0.4f), juce::dontSendNotification);
            };

            updateVisual(lowFreq, sync.hasB1Freq.load() ? sync.b1Freq.load() : targetDSP->b1Freq);
            updateVisual(lowGain, sync.hasB1Gain.load() ? sync.b1Gain.load() : targetDSP->b1Gain);
            updateVisual(lowQ,    sync.hasB1Q.load()    ? sync.b1Q.load()    : targetDSP->b1Q);

            updateVisual(highFreq, sync.hasB2Freq.load() ? sync.b2Freq.load() : targetDSP->b2Freq);
            updateVisual(highGain, sync.hasB2Gain.load() ? sync.b2Gain.load() : targetDSP->b2Gain);
            updateVisual(highQ,    sync.hasB2Q.load()    ? sync.b2Q.load()    : targetDSP->b2Q);
        }
        repaint(); 
    }

private:
    void drawResponseCurve(juce::Graphics& g, juce::Rectangle<int> bounds)
    {
        juce::Path p;
        bool first = true;
        float midY = (float)bounds.getCentreY();
        
        for (int x = 0; x <= bounds.getWidth(); x += 1) {
            float freq = 20.0f * std::pow(1000.0f, (float)x / (float)bounds.getWidth());
            float mag = targetDSP->getMagnitudeForFrequency(freq);
            float db = juce::Decibels::gainToDecibels(mag);
            float y = midY - (db / 24.0f) * (bounds.getHeight() / 2.0f);
            
            y = juce::jlimit((float)bounds.getY(), (float)bounds.getBottom(), y);
            
            if (first) { p.startNewSubPath((float)bounds.getX() + x, y); first = false; }
            else p.lineTo((float)bounds.getX() + x, y);
        }

        bool isBypassed = bypassBtn.getToggleState() == false;

        g.setColour(isBypassed ? juce::Colours::grey.withAlpha(0.2f) : juce::Colour(0, 200, 255).withAlpha(0.2f));
        g.strokePath(p, juce::PathStrokeType(3.0f));
        g.setColour(isBypassed ? juce::Colours::grey : juce::Colour(0, 220, 255));
        g.strokePath(p, juce::PathStrokeType(1.5f));
    }

    void drawFilterIcons(juce::Graphics& g) {
        auto drawIcon = [&](juce::Rectangle<int> b, int type, bool active) {
            g.setColour(active ? juce::Colour(0, 220, 255) : juce::Colours::grey.withAlpha(0.4f));
            juce::Path p;
            float x = (float)b.getX(), y = (float)b.getY(), w = (float)b.getWidth(), h = (float)b.getHeight();
            if (type == 0) { p.startNewSubPath(x, y+h); p.lineTo(x+w*0.7f, y+h); p.lineTo(x+w, y); } // HP
            else if (type == 1) { p.startNewSubPath(x, y+h*0.7f); p.lineTo(x+w*0.3f, y+h*0.7f); p.lineTo(x+w, y+h*0.3f); } // LS
            else if (type == 2) { p.startNewSubPath(x, y+h*0.5f); p.quadraticTo(x+w*0.5f, y, x+w, y+h*0.5f); } // Bell
            else if (type == 3) { p.startNewSubPath(x, y+h*0.3f); p.lineTo(x+w*0.7f, y+h*0.3f); p.lineTo(x+w, y+h*0.7f); } // HS
            else if (type == 4) { p.startNewSubPath(x, y); p.lineTo(x+w*0.3f, y); p.lineTo(x+w, y+h); } // LP
            g.strokePath(p, juce::PathStrokeType(1.5f));
        };

        drawIcon(b1HP.getBounds().reduced(5), 0, b1HP.getToggleState());
        drawIcon(b1Bell.getBounds().reduced(5), 2, b1Bell.getToggleState());
        drawIcon(b1LS.getBounds().reduced(5), 1, b1LS.getToggleState());

        drawIcon(b2LP.getBounds().reduced(5), 4, b2LP.getToggleState());
        drawIcon(b2Bell.getBounds().reduced(5), 2, b2Bell.getToggleState());
        drawIcon(b2HS.getBounds().reduced(5), 3, b2HS.getToggleState());
    }

    void updateDSPFromUI() {
        if (!targetDSP) return;
        targetDSP->setBand1Freq((float)lowFreq.getValue());
        targetDSP->setBand1Q((float)lowQ.getValue());
        targetDSP->setBand1Gain((float)lowGain.getValue());
        if (b1HP.getToggleState()) targetDSP->setBand1Type(ChannelEQ_DSP::HighPass);
        else if (b1LS.getToggleState()) targetDSP->setBand1Type(ChannelEQ_DSP::LowShelf);
        else targetDSP->setBand1Type(ChannelEQ_DSP::Bell);

        targetDSP->setBand2Freq((float)highFreq.getValue());
        targetDSP->setBand2Q((float)highQ.getValue());
        targetDSP->setBand2Gain((float)highGain.getValue());
        if (b2LP.getToggleState()) targetDSP->setBand2Type(ChannelEQ_DSP::LowPass);
        else if (b2HS.getToggleState()) targetDSP->setBand2Type(ChannelEQ_DSP::HighShelf);
        else targetDSP->setBand2Type(ChannelEQ_DSP::Bell);
    }

    void updateUIFromDSP() {
        lowFreq.setValue(targetDSP->getBand1Freq(), juce::dontSendNotification);
        lowQ.setValue(targetDSP->getBand1Q(), juce::dontSendNotification);
        lowGain.setValue(targetDSP->getBand1Gain(), juce::dontSendNotification);
        auto t1 = targetDSP->getBand1Type();
        b1HP.setToggleState(t1 == ChannelEQ_DSP::HighPass, juce::dontSendNotification);
        b1LS.setToggleState(t1 == ChannelEQ_DSP::LowShelf, juce::dontSendNotification);
        b1Bell.setToggleState(t1 == ChannelEQ_DSP::Bell, juce::dontSendNotification);

        highFreq.setValue(targetDSP->getBand2Freq(), juce::dontSendNotification);
        highQ.setValue(targetDSP->getBand2Q(), juce::dontSendNotification);
        highGain.setValue(targetDSP->getBand2Gain(), juce::dontSendNotification);
        auto t2 = targetDSP->getBand2Type();
        b2LP.setToggleState(t2 == ChannelEQ_DSP::LowPass, juce::dontSendNotification);
        b2HS.setToggleState(t2 == ChannelEQ_DSP::HighShelf, juce::dontSendNotification);
        b2Bell.setToggleState(t2 == ChannelEQ_DSP::Bell, juce::dontSendNotification);
        
        bypassBtn.setToggleState(!targetDSP->userBypass.load(std::memory_order_relaxed), juce::dontSendNotification);

        repaint();
    }

    FloatingValueSlider lowFreq, lowQ, lowGain, highFreq, highQ, highGain;
    juce::Label lowFreqLabel, lowQLabel, lowGainLabel, highFreqLabel, highQLabel, highGainLabel;
    juce::ShapeButton b1HP, b1Bell, b1LS, b2LP, b2Bell, b2HS;
    juce::TextButton bypassBtn;
    juce::Label trackNameLabel;

    ChannelEQ_DSP* targetDSP = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelEQ_Editor)
};
