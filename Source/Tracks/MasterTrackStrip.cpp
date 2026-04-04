#include "MasterTrackStrip.h"

MasterTrackStrip::MasterTrackStrip()
{
    // --- Paneo (Knob arriba del todo) ---
    addAndMakeVisible(panKnob);
    panKnob.setName("TrackKnob");
    panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panKnob.setRange(-1.0, 1.0);
    panKnob.setValue(0.0);
    panKnob.setDoubleClickReturnValue(true, 0.0);
    panKnob.onValueChange = [this] { if (masterTrack) masterTrack->setBalance((float)panKnob.getValue()); };

    // --- Label MASTER ---
    addAndMakeVisible(masterLabel);
    masterLabel.setText("MASTER", juce::dontSendNotification);
    masterLabel.setFont(juce::Font("Inter", 12.0f, juce::Font::bold));
    masterLabel.setColour(juce::Label::textColourId, juce::Colour(255, 200, 80));
    masterLabel.setJustificationType(juce::Justification::centred);

    // --- Mute ("M") ---
    addAndMakeVisible(muteBtn);
    muteBtn.setButtonText("M");
    muteBtn.setClickingTogglesState(true);
    muteBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 52));
    muteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker(0.2f));
    muteBtn.onClick = [this] { if (masterTrack) masterTrack->isMuted = muteBtn.getToggleState(); };

    // --- Solo ("S") ---
    addAndMakeVisible(soloBtn);
    soloBtn.setButtonText("S");
    soloBtn.setClickingTogglesState(true);
    soloBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 52));
    soloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow.darker(0.2f));
    soloBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    soloBtn.onClick = [this] { if (masterTrack) masterTrack->isSoloed = soloBtn.getToggleState(); };

    // --- Effects Button ("FX") ---
    addAndMakeVisible(effectsBtn);
    effectsBtn.setButtonText("FX");
    effectsBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 65, 70));
    effectsBtn.onClick = [this] { if (onTrackSelected) onTrackSelected(masterTrack); };

    // --- Medidor de pico (vertical) ---
    addAndMakeVisible(levelMeter);
    levelMeter.setHorizontal(false);

    // --- Volume Slider (OVERLAY sobre medidor) ---
    addAndMakeVisible(volSlider);
    volSlider.setSliderStyle(juce::Slider::LinearVertical);
    volSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volSlider.setRange(0.0, 1.5);
    volSlider.setValue(1.0);
    volSlider.setDoubleClickReturnValue(true, 1.0);
    // Hacer el slider "invisible" pero interactivo para que se vea el medidor debajo
    volSlider.setAlpha(0.7f); // Un poco de alpha para ver el handle pero no tapar todo
    volSlider.onValueChange = [this] { if (masterTrack) masterTrack->setVolume((float)volSlider.getValue()); };

    // --- Mouse Interception ---
    setInterceptsMouseClicks(true, true);
    masterLabel.setInterceptsMouseClicks(false, false);
    
    // Listeners for selection
    panKnob.addMouseListener(this, false);
    muteBtn.addMouseListener(this, false);
    soloBtn.addMouseListener(this, false);
    effectsBtn.addMouseListener(this, false);
    volSlider.addMouseListener(this, false);
    levelMeter.addMouseListener(this, false);

    levelMeter.addMouseListener(this, false);
}

MasterTrackStrip::~MasterTrackStrip()
{
}

void MasterTrackStrip::setMasterTrack(Track* t)
{
    masterTrack = t;
    levelMeter.setTrack(t);
    
    if (masterTrack != nullptr) {
        volSlider.setValue(masterTrack->getVolume(), juce::dontSendNotification);
        panKnob.setValue(masterTrack->getBalance(), juce::dontSendNotification);
        muteBtn.setToggleState(masterTrack->isMuted, juce::dontSendNotification);
        soloBtn.setToggleState(masterTrack->isSoloed, juce::dontSendNotification);
    }
}

Track* MasterTrackStrip::getMasterTrack() const
{
    return masterTrack;
}

// timerCallback eliminado - LevelMeter se auto-actualiza

void MasterTrackStrip::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background premium vertical
    juce::ColourGradient bg(
        juce::Colour(30, 28, 22), bounds.getTopLeft(),
        juce::Colour(20, 18, 15), bounds.getTopRight(), false);
    g.setGradientFill(bg);
    g.fillAll();

    // Accent line (top)
    g.setColour(juce::Colour(255, 200, 80));
    g.fillRect(0.0f, 0.0f, bounds.getWidth(), 3.0f);

    if (isSelected) {
        g.setColour(juce::Colour(255, 200, 80).withAlpha(0.05f));
        g.fillAll();
        g.setColour(juce::Colour(255, 200, 80).withAlpha(0.3f));
        g.drawRect(bounds.reduced(1.0f), 1.0f);
    }
}

void MasterTrackStrip::resized()
{
    auto area = getLocalBounds().reduced(5, 5);

    // 1. Pan en el tope
    panKnob.setBounds(area.removeFromTop(40).reduced(5, 0));

    // 2. Label MASTER
    masterLabel.setBounds(area.removeFromTop(20));

    area.removeFromTop(5);

    // 3. Botones M / S (en fila)
    auto btnRow = area.removeFromTop(30);
    muteBtn.setBounds(btnRow.removeFromLeft(btnRow.getWidth() / 2).reduced(2));
    soloBtn.setBounds(btnRow.reduced(2));

    area.removeFromTop(5);

    // 4. Botón FX
    effectsBtn.setBounds(area.removeFromTop(30).reduced(2, 0));

    area.removeFromTop(10);

    // 5. Volume & Meter (OVERLAY)
    // El Slider se pone exactamente encima del Meter
    auto meterArea = area; 
    levelMeter.setBounds(meterArea);
    volSlider.setBounds(meterArea);
}

void MasterTrackStrip::mouseDown(const juce::MouseEvent& e)
{
    if (onTrackSelected)
        onTrackSelected(masterTrack);
    
    isSelected = true;
    repaint();
}