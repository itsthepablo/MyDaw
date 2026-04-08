#include "MasterTrackStrip.h"
#include "../Mixer/Bridges/MixerParameterBridge.h"

MasterTrackStrip::MasterTrackStrip()
{
    // --- Label MASTER ---
    addAndMakeVisible(masterLabel);
    masterLabel.setText("MASTER", juce::dontSendNotification);
    masterLabel.setFont(juce::Font("Inter", 14.0f, juce::Font::bold));
    masterLabel.setColour(juce::Label::textColourId, juce::Colour(255, 200, 80));
    masterLabel.setJustificationType(juce::Justification::centredLeft);

    // --- Mute ("M") ---
    addAndMakeVisible(muteBtn);
    muteBtn.setButtonText("M");
    muteBtn.setClickingTogglesState(true);
    muteBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 52));
    muteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker(0.2f));
    muteBtn.onClick = [this] { if (masterTrack) MixerParameterBridge::setMuted(masterTrack, muteBtn.getToggleState()); };

    // --- Solo ("S") ---
    addAndMakeVisible(soloBtn);
    soloBtn.setButtonText("S");
    soloBtn.setClickingTogglesState(true);
    soloBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 52));
    soloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow.darker(0.2f));
    soloBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    soloBtn.onClick = [this] { if (masterTrack) MixerParameterBridge::setSoloed(masterTrack, soloBtn.getToggleState()); };

    // --- Paneo ---
    addAndMakeVisible(panKnob);
    panKnob.setName("TrackKnob");
    panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panKnob.setRange(-1.0, 1.0);
    panKnob.setValue(0.0);
    panKnob.setDoubleClickReturnValue(true, 0.0);
    panKnob.onValueChange = [this] { if (masterTrack) masterTrack->setBalance((float)panKnob.getValue()); };

    panKnob.valueToTextFormattingCallback = [](double value) -> juce::String {
        if (std::abs(value) < 0.01) return "Center";
        int percent = (int)(std::abs(value) * 100.0);
        return juce::String(percent) + (value < 0.0 ? "% L" : "% R");
    };

    // --- Volumen ---
    addAndMakeVisible(volKnob);
    volKnob.setName("TrackKnob");
    volKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    volKnob.setRange(0.0, 1.5);
    volKnob.setValue(1.0);
    volKnob.setDoubleClickReturnValue(true, 1.0);
    volKnob.onValueChange = [this] { if (masterTrack) masterTrack->setVolume((float)volKnob.getValue()); };

    volKnob.valueToTextFormattingCallback = [](double rawGain) -> juce::String {
        float db = juce::Decibels::gainToDecibels((float)rawGain, -100.0f);
        return db <= -100.0f ? "-inf dB" : juce::String(db, 1) + " dB";
    };

    // --- Effects Button ---
    addAndMakeVisible(effectsBtn);
    effectsBtn.setButtonText("Effects");
    effectsBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 65, 70));
    effectsBtn.onClick = [this] { if (onTrackSelected) onTrackSelected(masterTrack); };

    // --- Medidor de pico (Horizontal en el master track header) ---
    addAndMakeVisible(levelMeter);
    levelMeter.setHorizontal(false);

    // --- Mouse Interception ---
    setInterceptsMouseClicks(true, true);
    
    panKnob.addMouseListener(this, false);
    volKnob.addMouseListener(this, false);
    muteBtn.addMouseListener(this, false);
    soloBtn.addMouseListener(this, false);
    effectsBtn.addMouseListener(this, false);
}

MasterTrackStrip::~MasterTrackStrip() {}

void MasterTrackStrip::setMasterTrack(Track* t)
{
    masterTrack = t;
    levelMeter.setTrack(t);
    
    if (masterTrack != nullptr) {
        volKnob.setValue(MixerParameterBridge::getVolume(masterTrack), juce::dontSendNotification);
        panKnob.setValue(MixerParameterBridge::getBalance(masterTrack), juce::dontSendNotification);
        muteBtn.setToggleState(MixerParameterBridge::isMuted(masterTrack), juce::dontSendNotification);
        soloBtn.setToggleState(MixerParameterBridge::isSoloed(masterTrack), juce::dontSendNotification);
    }
}

Track* MasterTrackStrip::getMasterTrack() const { return masterTrack; }

void MasterTrackStrip::paint(juce::Graphics& g)
{
    // Fondo sólido similar a TrackControlPanel
    g.fillAll(juce::Colour(25, 27, 30));
    auto area = getLocalBounds();
    
    // --- Indicador de Color Master (Dorado/Naranja) ---
    auto colorBarArea = area.removeFromLeft(6).reduced(0, 4).translated(4, 0);
    juce::Colour masterColor = juce::Colour(255, 180, 50);
    g.setColour(masterColor);
    g.fillRoundedRectangle(colorBarArea.toFloat(), 3.0f);

    // --- Fondo de Contenido ---
    auto contentArea = area.withTrimmedLeft(8).reduced(4, 2);
    g.setColour(masterColor.withAlpha(0.12f));
    g.fillRoundedRectangle(contentArea.toFloat(), 4.0f);

    if (isSelected) {
        g.setColour(masterColor.withAlpha(0.2f));
        g.fillRoundedRectangle(contentArea.toFloat(), 4.0f);
        g.setColour(masterColor.withAlpha(0.5f));
        g.drawRoundedRectangle(contentArea.toFloat(), 4.0f, 1.5f);
    }

    // --- Línea Divisoria Superior (Integración con Playlist) ---
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawHorizontalLine(0, 0.0f, (float)getWidth());
}

void MasterTrackStrip::resized()
{
    auto area = getLocalBounds();
    // Margen para el indicador
    auto contentArea = area.withTrimmedLeft(18).reduced(4, 2);

    auto leftCol = contentArea.removeFromLeft(125);
    auto topRow = leftCol.removeFromTop(20);
    masterLabel.setBounds(topRow);
    
    leftCol.removeFromTop(2);
    
    auto kRow = leftCol.removeFromTop(28);
    muteBtn.setBounds(kRow.removeFromLeft(20).reduced(1));
    soloBtn.setBounds(kRow.removeFromLeft(20).reduced(1));
    panKnob.setBounds(kRow.removeFromLeft(35));
    volKnob.setBounds(kRow.removeFromLeft(35));
    
    auto botRow = leftCol.removeFromTop(18);
    effectsBtn.setBounds(botRow.reduced(2, 0));

    auto meterArea = contentArea.removeFromRight(12);
    levelMeter.setBounds(meterArea.reduced(0, 2));
}

void MasterTrackStrip::mouseDown(const juce::MouseEvent& e)
{
    if (onTrackSelected) onTrackSelected(masterTrack);
    isSelected = true;
    repaint();
}