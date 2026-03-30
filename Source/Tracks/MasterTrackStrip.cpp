#include "MasterTrackStrip.h"

MasterTrackStrip::MasterTrackStrip()
{
    // --- Label MASTER ---
    masterLabel.setText("MASTER", juce::dontSendNotification);
    masterLabel.setFont(juce::Font("Inter", 11.0f, juce::Font::bold));
    masterLabel.setColour(juce::Label::textColourId, juce::Colour(255, 200, 80));
    masterLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(masterLabel);

    // --- Botón FX ---
    fxButton.setButtonText("+ FX");
    fxButton.setTooltip("Inserta efectos y VST3 en el bus maestro");
    fxButton.setColour(juce::TextButton::buttonColourId,  juce::Colour(50, 55, 62));
    fxButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    fxButton.onClick = [this] { if (onOpenMasterFx) onOpenMasterFx(); };
    addAndMakeVisible(fxButton);

    // --- Slider de volumen ---
    volumeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    volumeSlider.setRange(0.0, 1.5);
    volumeSlider.setValue(1.0);
    volumeSlider.setDoubleClickReturnValue(true, 1.0);
    volumeSlider.setTooltip("Volumen del bus maestro");
    volumeSlider.setColour(juce::Slider::thumbColourId,        juce::Colour(255, 200, 80));
    volumeSlider.setColour(juce::Slider::trackColourId,        juce::Colour(70, 75, 82));
    volumeSlider.setColour(juce::Slider::backgroundColourId,   juce::Colour(30, 32, 36));
    volumeSlider.onValueChange = [this] {
        if (masterTrack != nullptr)
            masterTrack->setVolume((float)volumeSlider.getValue());
    };
    addAndMakeVisible(volumeSlider);

    // --- Medidor de pico (horizontal) ---
    addAndMakeVisible(levelMeter);

    startTimerHz(30);
}

MasterTrackStrip::~MasterTrackStrip()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void MasterTrackStrip::setMasterTrack(Track* t)
{
    masterTrack = t;
    if (masterTrack != nullptr)
        volumeSlider.setValue(masterTrack->getVolume(), juce::dontSendNotification);
}

Track* MasterTrackStrip::getMasterTrack() const 
{ 
    return masterTrack; 
}

void MasterTrackStrip::timerCallback()
{
    if (masterTrack != nullptr)
        levelMeter.setLevel(masterTrack->currentPeakLevelL, masterTrack->currentPeakLevelR);
}

void MasterTrackStrip::paint(juce::Graphics& g)
{
    // Fondo degradado dorado oscuro — estilo premium
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient bg(
        juce::Colour(35, 30, 20), bounds.getTopLeft(),
        juce::Colour(22, 20, 15), bounds.getBottomLeft(), false);
    g.setGradientFill(bg);
    g.fillRect(bounds);

    // Línea superior de separación dorada
    g.setColour(juce::Colour(255, 200, 80).withAlpha(0.6f));
    g.fillRect(0, 0, getWidth(), 2);

    // Línea dorada de acento izquierda
    g.setColour(juce::Colour(255, 200, 80));
    g.fillRect(0, 2, 4, getHeight() - 2);

    // Texto dBFS junto al medidor
    g.setColour(juce::Colours::white.withAlpha(0.5f));
    g.setFont(juce::Font(10.0f));
    g.drawText("0dB", getWidth() - 180, 4, 30, 14, juce::Justification::centred);
    g.drawText("-6", getWidth() - 145, 4, 25, 14, juce::Justification::centred);
    g.drawText("-12", getWidth() - 108, 4, 28, 14, juce::Justification::centred);
    g.drawText("-inf", getWidth() - 72, 4, 30, 14, juce::Justification::centred);
}

void MasterTrackStrip::resized()
{
    auto area = getLocalBounds().reduced(4, 6).withTrimmedLeft(6);

    // Label MASTER (izquierda)
    masterLabel.setBounds(area.removeFromLeft(60));
    area.removeFromLeft(4);

    // Botón FX
    fxButton.setBounds(area.removeFromLeft(55).reduced(0, 4));
    area.removeFromLeft(8);

    // Medidor de pico (derecha)
    levelMeter.setBounds(area.removeFromRight(120).reduced(0, 6));
    area.removeFromRight(4);

    // Slider de volumen (centro)
    volumeSlider.setBounds(area);
}