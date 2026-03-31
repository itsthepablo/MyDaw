#pragma once
#include <JuceHeader.h>
#include <functional>
#include <memory>
#include "MixerChannelUI.h" 

// Forward declaration de MixerComponent ya no es necesaria si usamos lambdas
// class MixerComponent;

// ==============================================================================
// MASTER CHANNEL UI
// ==============================================================================
class MasterChannelUI : public juce::Component {
public:
    MasterChannelUI(std::function<float()> getVol, std::function<void(float)> setVol) 
        : getVolumeCallback(getVol), setVolumeCallback(setVol), meter(nullptr) {
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

        // En un escenario real, pasaríamos el Track del Master aquí para ver niveles
        // meter = std::make_unique<AdvancedMeter>(masterTrack);
        // addAndMakeVisible(*meter);
    }

    ~MasterChannelUI() override { setLookAndFeel(nullptr); }

    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds();
        g.fillAll(juce::Colour(30, 33, 37)); 
        
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawRect(b, 2);

        // Franja de color especial para el Master (Rojo/Naranja)
        g.setColour(juce::Colours::orangered);
        g.fillRect(0, getHeight() - 6, getWidth(), 6);
        g.fillRect(0, 0, getWidth(), 4);
    }

    void resized() override {
        auto b = getLocalBounds().reduced(6);
        
        // Un Master suele ser más limpio arriba
        trackName.setBounds(b.removeFromTop(30));
        b.removeFromTop(10); // Gap
        
        auto faderArea = b.removeFromBottom(getHeight() * 0.7f);
        
        // Si tuviéramos meter
        if (meter) meter->setBounds(faderArea.removeFromRight(22));
        
        volumeSlider.setBounds(faderArea);
    }

private:
    std::function<float()> getVolumeCallback;
    std::function<void(float)> setVolumeCallback;

    MixerLookAndFeel mixerLAF;
    juce::Slider volumeSlider;
    juce::Label trackName;
    
    std::unique_ptr<AdvancedMeter> meter;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MasterChannelUI)
};