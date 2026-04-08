#include "MasterChannelUI.h"
#include "../LookAndFeel/MixerColours.h"

MasterChannelUI::MasterChannelUI(std::function<float()> getVol, std::function<void(float)> setVol) 
    : getVolumeCallback(getVol), setVolumeCallback(setVol)
{
    setLookAndFeel(&mixerLAF);

    volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
    volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    volumeSlider.setRange(0.0, 1.5);
    
    if (getVolumeCallback)
        volumeSlider.setValue(getVolumeCallback());

    volumeSlider.onValueChange = [this] { 
        if (setVolumeCallback)
            setVolumeCallback((float)volumeSlider.getValue()); 
    };
    addAndMakeVisible(volumeSlider);

    trackName.setText("MASTER", juce::dontSendNotification);
    trackName.setJustificationType(juce::Justification::centred);
    trackName.setColour(juce::Label::textColourId, juce::Colours::white);
    trackName.setFont(juce::Font(14.0f, juce::Font::bold));
    addAndMakeVisible(trackName);
}

MasterChannelUI::~MasterChannelUI() {
    setLookAndFeel(nullptr);
}

void MasterChannelUI::paint(juce::Graphics& g) {
    auto b = getLocalBounds();
    g.fillAll(getLookAndFeel().findColour(MixerColours::masterBackground)); 
    
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRect(b, 2);

    // Franja de color especial para el Master (Rojo/Naranja)
    g.setColour(juce::Colours::orangered);
    g.fillRect(0, getHeight() - 6, getWidth(), 6);
    g.fillRect(0, 0, getWidth(), 4);
}

void MasterChannelUI::resized() {
    auto b = getLocalBounds().reduced(6);
    
    trackName.setBounds(b.removeFromTop(30));
    b.removeFromTop(10); // Gap
    
    auto faderArea = b.removeFromBottom(getHeight() * 0.7f);
    
    if (meter) meter->setBounds(faderArea.removeFromRight(22));
    
    volumeSlider.setBounds(faderArea);
}
