#include "GainStationPanel.h"

GainStationPanel::GainStationPanel() {
    addAndMakeVisible(meter);

    addAndMakeVisible(preGainKnob);
    preGainKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    preGainKnob.setRange(0.0, 2.0, 0.01);
    preGainKnob.setValue(1.0, juce::dontSendNotification);
    preGainKnob.setTooltip("Pre-Gain (Input Trim): Ajusta la entrada antes de los FX.");
    preGainKnob.onValueChange = [this] {
        if (activeBridge) activeBridge->setPreGain((float)preGainKnob.getValue());
    };

    addAndMakeVisible(postGainKnob);
    postGainKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    postGainKnob.setRange(0.0, 2.0, 0.01);
    postGainKnob.setValue(1.0, juce::dontSendNotification);
    postGainKnob.setTooltip("Post-Gain (Output Trim): Compensa la ganancia tras los FX.");
    postGainKnob.onValueChange = [this] {
        if (activeBridge) activeBridge->setPostGain((float)postGainKnob.getValue());
    };

    addAndMakeVisible(phaseBtn);
    phaseBtn.setButtonText(juce::CharPointer_UTF8("\xc3\x98"));
    phaseBtn.setClickingTogglesState(true);
    phaseBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker(0.2f));
    phaseBtn.setTooltip("Invertir Polaridad (Fase)");
    phaseBtn.onClick = [this] {
        if (activeBridge) activeBridge->setPhaseInverted(phaseBtn.getToggleState());
    };

    addAndMakeVisible(monoBtn);
    monoBtn.setButtonText("MONO");
    monoBtn.setClickingTogglesState(true);
    monoBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange.darker(0.2f));
    monoBtn.setTooltip("Sumar a Mono (L+R)");
    monoBtn.onClick = [this] {
        if (activeBridge) activeBridge->setMonoActive(monoBtn.getToggleState());
    };

    addAndMakeVisible(matchBtn);
    matchBtn.setButtonText("MATCH");
    matchBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgreen.darker(0.5f));
    matchBtn.setTooltip("Iguala el volumen de salida con el de entrada automáticamente.");
    matchBtn.onClick = [this] { performGainMatch(); };
}

GainStationPanel::~GainStationPanel() {}

void GainStationPanel::setBridge(GainStationBridge* b) {
    activeBridge = b;
    meter.setBridge(b);
    if (activeBridge) {
        preGainKnob.setValue(activeBridge->getPreGain(), juce::dontSendNotification);
        postGainKnob.setValue(activeBridge->getPostGain(), juce::dontSendNotification);
        phaseBtn.setToggleState(activeBridge->isPhaseInverted(), juce::dontSendNotification);
        monoBtn.setToggleState(activeBridge->isMonoActive(), juce::dontSendNotification);
    }
    repaint();
}

void GainStationPanel::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(30, 33, 36));

    // Líneas organizativas internas
    g.setColour(juce::Colour(50, 53, 56));
    g.drawHorizontalLine(getHeight() / 2, 10.0f, (float)getWidth() - 10.0f);
    g.drawVerticalLine(getWidth() / 2, 10.0f, (float)getHeight() - 10.0f);

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

void GainStationPanel::resized() {
    auto area = getLocalBounds().reduced(6);
    meter.setBounds(area.removeFromLeft(getWidth() / 2 - 6));
    area.removeFromLeft(6); 
    auto rightArea = area;
    int halfH = rightArea.getHeight() / 2;

    auto preArea = rightArea.removeFromTop(halfH);
    auto preKnobArea = preArea.removeFromTop((int)(preArea.getHeight() * 0.55f));
    preGainKnob.setBounds(preKnobArea.reduced(6, 2));
    preArea.removeFromTop(18); 
    phaseBtn.setBounds(preArea.removeFromTop(20).reduced(12, 0));

    auto postArea = rightArea;
    auto postKnobArea = postArea.removeFromTop((int)(postArea.getHeight() * 0.55f));
    postGainKnob.setBounds(postKnobArea.reduced(6, 2));
    postArea.removeFromTop(18); 
    auto postBtnsArea = postArea.removeFromBottom(22);
    monoBtn.setBounds(postBtnsArea.removeFromLeft(postBtnsArea.getWidth() / 2).reduced(2, 0));
    matchBtn.setBounds(postBtnsArea.reduced(2, 0));
}

void GainStationPanel::performGainMatch() {
    if (!activeBridge) return;
    float inLufs = activeBridge->getInputLUFS();
    float outLufs = activeBridge->getOutputLUFS();

    if (inLufs > -60.0f && outLufs > -60.0f) {
        float diffDb = inLufs - outLufs;
        float currentGainDb = juce::Decibels::gainToDecibels((float)postGainKnob.getValue());
        float newGain = juce::Decibels::decibelsToGain(currentGainDb + diffDb);

        if (newGain > 2.0f) newGain = 2.0f;
        if (newGain < 0.0f) newGain = 0.0f;

        postGainKnob.setValue(newGain, juce::sendNotification);
    }
}
