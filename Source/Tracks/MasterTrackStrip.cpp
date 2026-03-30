#include "MasterTrackStrip.h"

MasterTrackStrip::MasterTrackStrip()
{
    // --- Label MASTER ---
    masterLabel.setText("MASTER", juce::dontSendNotification);
    masterLabel.setFont(juce::Font("Inter", 11.0f, juce::Font::bold));
    masterLabel.setColour(juce::Label::textColourId, juce::Colour(255, 200, 80));
    masterLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(masterLabel);

    // --- Knob de volumen (Reemplaza al Slider Horizontal y al Botón FX) ---
    volumeSlider.setName("TrackKnob"); // Para que herede el look & feel global de los otros knobs si lo tienes configurado
    volumeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    volumeSlider.setRange(0.0, 1.5);
    volumeSlider.setValue(1.0);
    volumeSlider.setDoubleClickReturnValue(true, 1.0);
    volumeSlider.setTooltip("Volumen del bus maestro");
    volumeSlider.setColour(juce::Slider::thumbColourId, juce::Colour(255, 200, 80));
    volumeSlider.setColour(juce::Slider::trackColourId, juce::Colour(70, 75, 82));
    volumeSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(30, 32, 36));
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
}

void MasterTrackStrip::resized()
{
    auto area = getLocalBounds().reduced(4, 6).withTrimmedLeft(6);

    // Label MASTER (izquierda)
    masterLabel.setBounds(area.removeFromLeft(60));
    area.removeFromLeft(4);

    // Knob de volumen (Ocupa el lugar del antiguo botón FX)
    volumeSlider.setBounds(area.removeFromLeft(45).reduced(0, 2));
    area.removeFromLeft(8);

    // Medidor de pico (derecha)
    levelMeter.setBounds(area.removeFromRight(120).reduced(0, 6));

    // El espacio restante en 'area' (centro) queda vacío hasta que definas el diseño final.
}