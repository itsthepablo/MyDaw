#pragma once
#include <JuceHeader.h>
#include "../BaseEffect.h"
#include "OrionSynth.h"
#include <atomic>
#include <map>

// ==============================================================================
struct OrionDebugStats {
    std::atomic<bool> midiActivity { false };
    std::atomic<int> activeVoices { 0 };
    std::atomic<float> internalLevel { 0.0f };
};

// ==============================================================================
class OrionEditor : public juce::Component, public juce::Timer, public juce::MidiKeyboardState::Listener {
public:
    OrionEditor(std::map<juce::String, std::atomic<float>*>& params, juce::MidiKeyboardState& kState, OrionDebugStats& debug, juce::Synthesiser& s) 
        : parameters(params), keyboardComponent(kState, juce::MidiKeyboardComponent::horizontalKeyboard), stats(debug), synth(s), keyboardState(kState)
    {
        keyboardState.addListener(this);

        // Estética del teclado
        keyboardComponent.setAvailableRange(21, 108);
        keyboardComponent.setKeyWidth(20);
        
        setupSlider(cutoffSlider, "Cutoff", 20.0f, 20000.0f, true);
        setupSlider(resSlider, "Res", 0.0f, 1.0f, false);
        setupSlider(attackSlider, "A", 0.001f, 5.0f, false);
        setupSlider(decaySlider, "D", 0.01f, 5.0f, false);
        setupSlider(sustainSlider, "S", 0.0f, 1.0f, false);
        setupSlider(releaseSlider, "R", 0.01f, 5.0f, false);

        // Teclado virtual
        addAndMakeVisible(keyboardComponent);

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
        setFocusContainer(true);
        startTimer(33); // 30 FPS para la depuración
    }

    ~OrionEditor() override {
        keyboardState.removeListener(this);
    }

    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override {
        synth.noteOn(midiChannel, midiNoteNumber, velocity);
        stats.midiActivity.store(true);
    }

    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override {
        synth.noteOff(midiChannel, midiNoteNumber, velocity, true);
    }

    void timerCallback() override {
        repaint();
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
        
        // Fondo integrado premium
        g.setColour(juce::Colour(25, 28, 32));
        g.fillRoundedRectangle(area.reduced(1).toFloat(), 4.0f);
        
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.drawRoundedRectangle(area.reduced(1).toFloat(), 4.0f, 1.0f);

        // --- BARRA DE CUALQUIER DEBUG SUPERIOR ---
        auto debugArea = area.removeFromTop(18).reduced(5, 0);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(debugArea.toFloat(), 2.0f);

        // MIDI Indicator
        bool hasMidi = stats.midiActivity.exchange(false);
        g.setColour(hasMidi ? juce::Colours::lime : juce::Colours::darkgrey);
        g.fillEllipse(debugArea.removeFromLeft(10).reduced(2).toFloat());
        
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(9.0f);
        g.drawText("MIDI", 15, debugArea.getY(), 30, 18, juce::Justification::left);

        // Voice Count
        int vCount = stats.activeVoices.load();
        g.drawText("VOICES: " + juce::String(vCount), 50, debugArea.getY(), 60, 18, juce::Justification::left);

        // Internal Level
        float level = stats.internalLevel.load();
        auto meterArea = debugArea.removeFromRight(100).reduced(0, 5);
        g.setColour(juce::Colours::black);
        g.fillRect(meterArea);
        g.setColour(juce::Colours::cyan.withAlpha(0.8f));
        g.fillRect(meterArea.removeFromLeft((int)(meterArea.getWidth() * juce::jlimit(0.0f, 1.0f, level))));

        // Separador para el teclado
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawHorizontalLine(area.getHeight() - 60, 2, area.getWidth() - 2);

        g.setColour(juce::Colours::cyan.withAlpha(0.6f));
        g.setFont(juce::Font("Sans-Serif", 10.0f, juce::Font::bold));
        g.drawText("OSC", 10, debugArea.getBottom() + 5, 40, 15, juce::Justification::left);
        g.drawText("ADSR", 110, debugArea.getBottom() + 5, 40, 15, juce::Justification::left);
        
        // Etiquetas pequeñas para sliders
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.setFont(9.0f);
        g.drawText("CUT", 15, 65, 30, 10, juce::Justification::centred);
        g.drawText("RES", 60, 65, 30, 10, juce::Justification::centred);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(5);
        auto keyboardArea = area.removeFromBottom(60);
        
        area.removeFromTop(20); // Espacio para el panel de debug
        
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
    OrionDebugStats& stats;
    juce::Slider cutoffSlider, resSlider;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::MidiKeyboardComponent keyboardComponent;
    juce::Synthesiser& synth;
    juce::MidiKeyboardState& keyboardState;
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

        editor = std::make_unique<OrionEditor>(paramPtrs, keyboardState, debugStats, synth);
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
        processBlock(buffer, midiMessages, nullptr); // Redirigir siempre a la versión completa
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, const juce::AudioBuffer<float>* sidechainBuffer) override {
        if (bypassed) return;

        // --- PRUEBA DE VIDA (Aparecerá en el vúmetro un pequeño ruido si el hilo corre) ---
        debugStats.internalLevel.store(0.0001f); 

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

        // --- PROCESAMIENTO DE PATRONES MIDI ---
        if (!midiMessages.isEmpty()) debugStats.midiActivity.store(true);

        // Renderizado del sintetizador (la UI maneja sus propias notas mediante synth.noteOn directo)
        synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

        // Actualizar contador de voces activas para el diagnóstico
        int liveVoices = 0;
        for (int i = 0; i < synth.getNumVoices(); ++i) {
            if (synth.getVoice(i)->isVoiceActive()) liveVoices++;
        }
        debugStats.activeVoices.store(liveVoices);

        // Medir salida real para el vúmetro (incluyendo un pequeño offset para "life-sign")
        float mag = buffer.getMagnitude(0, buffer.getNumSamples());
        debugStats.internalLevel.store(juce::jmax(0.001f, mag));
    }

    int getLatencySamples() const override { return 0; }
    bool isNative() const override { return true; }
    juce::Component* getCustomEditor() override { return editor.get(); }

private:
    bool bypassed = false;
    juce::Synthesiser synth;
    juce::MidiKeyboardState keyboardState;
    OrionDebugStats debugStats;
    
    // Contenedores de parámetros compatibles con Real-Time
    std::map<juce::String, std::atomic<float>> paramValues;
    std::map<juce::String, std::atomic<float>*> paramPtrs;

    std::unique_ptr<OrionEditor> editor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrionPlugin)
};
