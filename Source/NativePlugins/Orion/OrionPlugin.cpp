#include "OrionPlugin.h"
#include "OrionEditor.h"

// ==============================================================================
// ORION EDITOR IMPLEMENTATION
// ==============================================================================

void OrionEditor::handleNoteOn(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) {
    synth.noteOn(midiChannel, midiNoteNumber, velocity);
    stats.midiActivity.store(true);
}

void OrionEditor::handleNoteOff(juce::MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) {
    synth.noteOff(midiChannel, midiNoteNumber, velocity, true);
}

void OrionEditor::timerCallback() {
    repaint();
}

void OrionEditor::setupSlider(juce::Slider& s, juce::String name, float min, float max, bool isLog) {
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

void OrionEditor::paint(juce::Graphics& g) {
    auto area = getLocalBounds();
    
    g.setColour(juce::Colour(25, 28, 32));
    g.fillRoundedRectangle(area.reduced(1).toFloat(), 4.0f);
    
    g.setColour(juce::Colours::black.withAlpha(0.2f));
    g.drawRoundedRectangle(area.reduced(1).toFloat(), 4.0f, 1.0f);

    auto debugArea = area.removeFromTop(18).reduced(5, 0);
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.fillRoundedRectangle(debugArea.toFloat(), 2.0f);

    bool hasMidi = stats.midiActivity.exchange(false);
    g.setColour(hasMidi ? juce::Colours::lime : juce::Colours::darkgrey);
    g.fillEllipse(debugArea.removeFromLeft(10).reduced(2).toFloat());
    
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(9.0f);
    g.drawText("MIDI", 15, debugArea.getY(), 30, 18, juce::Justification::left);

    int vCount = stats.activeVoices.load();
    g.drawText("VOICES: " + juce::String(vCount), 50, debugArea.getY(), 60, 18, juce::Justification::left);

    float level = stats.internalLevel.load();
    auto meterArea = debugArea.removeFromRight(100).reduced(0, 5);
    g.setColour(juce::Colours::black);
    g.fillRect(meterArea);
    g.setColour(juce::Colours::cyan.withAlpha(0.8f));
    g.fillRect(meterArea.removeFromLeft((int)(meterArea.getWidth() * juce::jlimit(0.0f, 1.0f, level))));

    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawHorizontalLine(area.getHeight() - 60, 2, area.getWidth() - 2);

    g.setColour(juce::Colours::cyan.withAlpha(0.6f));
    g.setFont(juce::Font("Sans-Serif", 10.0f, juce::Font::bold));
    g.drawText("OSC", 10, debugArea.getBottom() + 5, 40, 15, juce::Justification::left);
    g.drawText("ADSR", 110, debugArea.getBottom() + 5, 40, 15, juce::Justification::left);
    
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(9.0f);
    g.drawText("CUT", 15, 65, 30, 10, juce::Justification::centred);
    g.drawText("RES", 60, 65, 30, 10, juce::Justification::centred);
}

void OrionEditor::resized() {
    auto area = getLocalBounds().reduced(5);
    auto keyboardArea = area.removeFromBottom(60);
    
    area.removeFromTop(20); 
    
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

// ==============================================================================
// ORION PLUGIN IMPLEMENTATION
// ==============================================================================

OrionPlugin::OrionPlugin() {
    setIsInstrument(true);
    debugStats = std::make_unique<OrionDebugStats>();
    
    paramValues["Cutoff"].store(2000.0f);
    paramValues["Res"].store(0.1f);
    paramValues["A"].store(0.01f);
    paramValues["D"].store(0.1f);
    paramValues["S"].store(0.7f);
    paramValues["R"].store(0.3f);

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

    editor = std::make_unique<OrionEditor>(paramPtrs, keyboardState, *debugStats, synth);
}

void OrionPlugin::showWindow(TrackContainer*) {}

void OrionPlugin::reset() {
    synth.allNotesOff(0, false);
}

void OrionPlugin::prepareToPlay(double sampleRate, int samplesPerBlock) {
    synth.setCurrentPlaybackSampleRate(sampleRate);
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)samplesPerBlock, 2 };
    for (int i = 0; i < synth.getNumVoices(); ++i) {
        if (auto* voice = dynamic_cast<OrionVoice*>(synth.getVoice(i))) {
            voice->prepare(spec);
        }
    }
}

void OrionPlugin::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, const juce::AudioBuffer<float>* sidechainBuffer) {
    if (bypassed) return;

    debugStats->internalLevel.store(0.0001f); 

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

    if (!midiMessages.isEmpty()) debugStats->midiActivity.store(true);

    synth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    int liveVoices = 0;
    for (int i = 0; i < synth.getNumVoices(); ++i) {
        if (synth.getVoice(i)->isVoiceActive()) liveVoices++;
    }
    debugStats->activeVoices.store(liveVoices);

    float mag = buffer.getMagnitude(0, buffer.getNumSamples());
    debugStats->internalLevel.store(juce::jmax(0.001f, mag));
}

juce::Component* OrionPlugin::getCustomEditor() {
    return editor.get();
}
