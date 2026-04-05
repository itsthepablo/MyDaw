#pragma once
#include <JuceHeader.h>
#include "../BaseEffect.h"
#include "OrionSynth.h"
#include <atomic>
#include <map>

// ==============================================================================
class OrionEditor : public juce::Component {
public:
    OrionEditor(std::map<juce::String, std::atomic<float>*>& params, juce::MidiKeyboardState& kState) 
        : parameters(params), keyboardComponent(kState, juce::MidiKeyboardComponent::horizontalKeyboard) 
    {
        setupSlider(cutoffSlider, "Cutoff", 20.0f, 20000.0f, true);
        setupSlider(resSlider, "Res", 0.0f, 1.0f, false);
        setupSlider(attackSlider, "A", 0.001f, 5.0f, false);
        setupSlider(decaySlider, "D", 0.01f, 5.0f, false);
        setupSlider(sustainSlider, "S", 0.0f, 1.0f, false);
        setupSlider(releaseSlider, "R", 0.01f, 5.0f, false);

        // Teclado virtual
        addAndMakeVisible(keyboardComponent);
        keyboardComponent.setAvailableRange(21, 108); // Rango de piano estándar
        keyboardComponent.setKeyWidth(20);

        // Mapeo manual de teclado de PC (Professional & Robust)
        // Fila 1 (Blancas): A, S, D, F, G, H, J, K -> Do, Re, Mi, Fa, Sol, La, Si, Do
        // Fila 2 (Negras): W, E, T, Y, U -> Do#, Re#, Fa#, Sol#, La#
        keyboardComponent.setKeyPressForNote(juce::KeyPress('a'), 60);
        keyboardComponent.setKeyPressForNote(juce::KeyPress('w'), 61);
        keyboardComponent.setKeyPressForNote(juce::KeyPress('s'), 62);
        keyboardComponent.setKeyPressForNote(juce::KeyPress('e'), 63);
        keyboardComponent.setKeyPressForNote(juce::KeyPress('d'), 64);
        keyboardComponent.setKeyPressForNote(juce::KeyPress('f'), 65);
        keyboardComponent.setKeyPressForNote(juce::KeyPress('t'), 66);
        keyboardComponent.setKeyPressForNote(juce::KeyPress('g'), 67);
        keyboardComponent.setKeyPressForNote(juce::KeyPress('y'), 68);
        keyboardComponent.setKeyPressForNote(juce::KeyPress('h'), 69);
        keyboardComponent.setKeyPressForNote(juce::KeyPress('u'), 70);
        keyboardComponent.setKeyPressForNote(juce::KeyPress('j'), 71);
        keyboardComponent.setKeyPressForNote(juce::KeyPress('k'), 72);

        setWantsKeyboardFocus(true);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        grabKeyboardFocus();
        juce::Component::mouseDown(e);
    }

    bool keyPressed(const juce::KeyPress& key) override {
        return keyboardComponent.keyPressed(key);
    }

    bool keyStateChanged(bool isKeyDown) override {
        return keyboardComponent.keyStateChanged(isKeyDown);
    }

    void setupSlider(juce::Slider& s, juce::String name, float min, float max, bool isLog) {
        addAndMakeVisible(s);
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.setRange(min, max);
        if (isLog) s.setSkewFactorFromMidPoint(1000.0f);
        
        if (parameters.count(name)) {
            s.setValue(parameters[name]->load());
        }

        s.onValueChange = [this, &s, name] {
            if (parameters.count(name)) {
                parameters[name]->store((float)s.getValue());
            }
        };
    }

    void paint(juce::Graphics& g) override {
        auto area = getLocalBounds();
        
        // Separador para el teclado
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawHorizontalLine(area.getHeight() - 60, 0, area.getWidth());

        g.setColour(juce::Colours::cyan.withAlpha(0.4f));
        g.setFont(juce::Font(10.0f, juce::Font::bold));
        g.drawText("OSC", 10, 5, 40, 15, juce::Justification::left);
        g.drawText("ADSR", 110, 5, 40, 15, juce::Justification::left);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(5);
        auto keyboardArea = area.removeFromBottom(60);
        
        area.removeFromTop(15); 
        
        auto oscArea = area.removeFromLeft(90);
        cutoffSlider.setBounds(oscArea.removeFromLeft(45).reduced(2));
        resSlider.setBounds(oscArea.reduced(2));

        area.removeFromLeft(10); 

        int adsrW = area.getWidth() / 4;
        attackSlider.setBounds(area.removeFromLeft(adsrW).reduced(2));
        decaySlider.setBounds(area.removeFromLeft(adsrW).reduced(2));
        sustainSlider.setBounds(area.removeFromLeft(adsrW).reduced(2));
        releaseSlider.setBounds(area.reduced(2));

        keyboardComponent.setBounds(keyboardArea);
    }

private:
    std::map<juce::String, std::atomic<float>*>& parameters;
    juce::Slider cutoffSlider, resSlider;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::MidiKeyboardComponent keyboardComponent;
};

// ==============================================================================
class OrionPlugin : public BaseEffect {
public:
    OrionPlugin() {
        setIsInstrument(true);
        
        // Inicializar parámetros manualmente para evitar errores de atomic copy
        paramValues["Cutoff"].store(2000.0f);
        paramValues["Res"].store(0.1f);
        paramValues["A"].store(0.01f);
        paramValues["D"].store(0.1f);
        paramValues["S"].store(0.7f);
        paramValues["R"].store(0.3f);

        // Mapear punteros para el editor
        paramPtrs["Cutoff"] = &paramValues["Cutoff"];
        paramPtrs["Res"] = &paramValues["Res"];
        paramPtrs["A"] = &paramValues["A"];
        paramPtrs["D"] = &paramValues["D"];
        paramPtrs["S"] = &paramValues["S"];
        paramPtrs["R"] = &paramValues["R"];

        for (int i = 0; i < 16; ++i) {
            synth.addVoice(new OrionVoice());
        }
        synth.addSound(new OrionSound());

        editor = std::make_unique<OrionEditor>(paramPtrs, keyboardState);
    }

    bool isLoaded() const override { return true; }
    juce::String getLoadedPluginName() const override { return "Orion"; }
    bool isBypassed() const override { return bypassed; }
    void setBypassed(bool b) override { bypassed = b; }

    void showWindow(TrackContainer*) override {}

    void prepareToPlay(double sampleRate, int samplesPerBlock) override {
        synth.setCurrentPlaybackSampleRate(sampleRate);
        juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, 2 };
        for (int i = 0; i < synth.getNumVoices(); ++i) {
            if (auto* voice = dynamic_cast<OrionVoice*>(synth.getVoice(i))) {
                voice->prepare(spec);
            }
        }
    }

    void reset() override {
        synth.allNotesOff(0, false);
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override {
        if (bypassed) return;

        float a = paramValues["A"].load();
        float d = paramValues["D"].load();
        float s = paramValues["S"].load();
        float r = paramValues["R"].load();
        float cutoff = paramValues["Cutoff"].load();
        float res = paramValues["Res"].load();

        for (int i = 0; i < synth.getNumVoices(); ++i) {
            if (auto* v = dynamic_cast<OrionVoice*>(synth.getVoice(i))) {
                v->updateParams(a, d, s, r, cutoff, res, 1, 1);
            }
        }

        // --- MIDI KEYBOARD INTEGRATION ---
        keyboardState.processNextMidiBuffer(midiMessages, 0, buffer.getNumSamples(), false);

        synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    }

    int getLatencySamples() const override { return 0; }
    bool isNative() const override { return true; }
    juce::Component* getCustomEditor() override { return editor.get(); }

private:
    bool bypassed = false;
    juce::Synthesiser synth;
    juce::MidiKeyboardState keyboardState;
    
    // Contenedores de parámetros compatibles con Real-Time
    std::map<juce::String, std::atomic<float>> paramValues;
    std::map<juce::String, std::atomic<float>*> paramPtrs;

    std::unique_ptr<OrionEditor> editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrionPlugin)
};
