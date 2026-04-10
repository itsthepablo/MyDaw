#pragma once
#include <JuceHeader.h>
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

    void handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void timerCallback() override;

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

    void setupSlider(juce::Slider& s, juce::String name, float min, float max, bool isLog);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::map<juce::String, std::atomic<float>*>& parameters;
    OrionDebugStats& stats;
    juce::Slider cutoffSlider, resSlider;
    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::MidiKeyboardComponent keyboardComponent;
    juce::Synthesiser& synth;
    juce::MidiKeyboardState& keyboardState;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrionEditor)
};
