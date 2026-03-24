#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"

// ==============================================================================
// 1. LOOK & FEEL Y COMPONENTES
// ==============================================================================
class ModernDialLookAndFeel : public juce::LookAndFeel_V4 {
public:
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override {
        auto dialBounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
        auto radius = (float)juce::jmin(dialBounds.getWidth() / 2, dialBounds.getHeight() / 2);
        auto centreX = dialBounds.getCentreX();
        auto centreY = dialBounds.getCentreY();
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(juce::Colour(30, 33, 36));
        g.fillEllipse(rx, ry, rw, rw);
        g.setColour(juce::Colours::orange);
        juce::Path p;
        p.addRectangle(-1.0f, -radius, 2.0f, radius * 0.7f);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.fillPath(p);
    }
};

class DualPanBar : public juce::Component {
public:
    juce::Slider sliderL, sliderR;
    DualPanBar() {
        addAndMakeVisible(sliderL); addAndMakeVisible(sliderR);
        sliderL.setSliderStyle(juce::Slider::LinearHorizontal);
        sliderR.setSliderStyle(juce::Slider::LinearHorizontal);
        sliderL.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        sliderR.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        sliderL.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        sliderR.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
    }
    void resized() override {
        sliderL.setBounds(0, 0, getWidth(), getHeight() / 2);
        sliderR.setBounds(0, getHeight() / 2, getWidth(), getHeight() / 2);
    }
};

class MixerMeter : public juce::Component, public juce::Timer {
public:
    MixerMeter(Track* t) : track(t) { startTimerHz(30); }
    void timerCallback() override { repaint(); }
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colours::black.withAlpha(0.6f));
        if (!track) return;
        float lufs = track->postLoudness.getShortTerm();
        float mapped = juce::jmap(lufs, -70.0f, 0.0f, (float)getHeight(), 0.0f);
        mapped = juce::jlimit(0.0f, (float)getHeight(), mapped);
        g.setColour(juce::Colours::limegreen);
        g.fillRect(0.0f, mapped, (float)getWidth(), getHeight() - mapped);
    }
private:
    Track* track;
};

// ==============================================================================
// 2. MIXER CHANNEL UI (Tu diseño con ESCALADO UNIFORME)
// ==============================================================================
class MixerChannelUI : public juce::Component {
public:
    MixerChannelUI(Track* t) : track(t), peakMeter_1(t) {
        setLookAndFeel(&globalLAF);

        addAndMakeVisible(peakMeter_1);

        midSideKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        midSideKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(midSideKnob);

        volumeSlider.setSliderStyle(juce::Slider::LinearVertical);
        volumeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 20);
        volumeSlider.setColour(juce::Slider::thumbColourId, juce::Colours::orange);
        volumeSlider.setRange(0.0, 1.0);
        volumeSlider.setValue(track->getVolume());
        volumeSlider.onValueChange = [this] { if (track) track->setVolume(volumeSlider.getValue()); };
        addAndMakeVisible(volumeSlider);

        panKnobNormal.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panKnobNormal.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panKnobNormal.setRange(-1.0, 1.0);
        panKnobNormal.setValue(track->getBalance());
        panKnobNormal.onValueChange = [this] { if (track) track->setBalance(panKnobNormal.getValue()); };
        addAndMakeVisible(panKnobNormal);

        addAndMakeVisible(dualPanBar);

        reversePolarityBtn.setButtonText("O");
        addAndMakeVisible(reversePolarityBtn);

        swapLRBtn.setButtonText("L/R");
        addAndMakeVisible(swapLRBtn);

        bypassEffectsBtn.setButtonText("FX");
        addAndMakeVisible(bypassEffectsBtn);

        muteSoloBtn.setButtonText("M/S");
        muteSoloBtn.setClickingTogglesState(true);
        muteSoloBtn.setToggleState(track->isMuted, juce::dontSendNotification);
        muteSoloBtn.onClick = [this] { if (track) track->isMuted = muteSoloBtn.getToggleState(); };
        addAndMakeVisible(muteSoloBtn);

        trackName.setText(track->getName(), juce::dontSendNotification);
        trackName.setJustificationType(juce::Justification::centred);
        trackName.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(trackName);
    }

    ~MixerChannelUI() override { setLookAndFeel(nullptr); }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(40, 43, 48));
        g.setColour(juce::Colour(20, 22, 25));
        g.drawRect(getLocalBounds(), 2);
    }

    void resized() override {
        // ===================================================================
        // LA CLAVE: UN SOLO MULTIPLICADOR (scale) PARA TODO
        // Como el canal ya fue forzado a mantener su relación de aspecto,
        // getHeight() / 600.0f nos da el porcentaje exacto de encogimiento.
        // ===================================================================
        float scale = getHeight() / 600.0f;

        // Multiplicamos TODAS tus coordenadas por ese único número (evitando deformaciones)
        peakMeter_1.setBounds(juce::roundToInt(10 * scale), juce::roundToInt(100 * scale), juce::roundToInt(50 * scale), juce::roundToInt(490 * scale));
        midSideKnob.setBounds(juce::roundToInt(10 * scale), juce::roundToInt(20 * scale), juce::roundToInt(40 * scale), juce::roundToInt(40 * scale));
        volumeSlider.setBounds(juce::roundToInt(60 * scale), juce::roundToInt(220 * scale), juce::roundToInt(50 * scale), juce::roundToInt(370 * scale));
        panKnobNormal.setBounds(juce::roundToInt(70 * scale), juce::roundToInt(20 * scale), juce::roundToInt(40 * scale), juce::roundToInt(40 * scale));
        dualPanBar.setBounds(juce::roundToInt(10 * scale), juce::roundToInt(70 * scale), juce::roundToInt(100 * scale), juce::roundToInt(20 * scale));
        reversePolarityBtn.setBounds(juce::roundToInt(70 * scale), juce::roundToInt(100 * scale), juce::roundToInt(30 * scale), juce::roundToInt(30 * scale));
        swapLRBtn.setBounds(juce::roundToInt(70 * scale), juce::roundToInt(140 * scale), juce::roundToInt(30 * scale), juce::roundToInt(30 * scale));
        bypassEffectsBtn.setBounds(juce::roundToInt(70 * scale), juce::roundToInt(180 * scale), juce::roundToInt(30 * scale), juce::roundToInt(30 * scale));
        muteSoloBtn.setBounds(juce::roundToInt(50 * scale), 0, juce::roundToInt(20 * scale), juce::roundToInt(20 * scale));

        trackName.setBounds(0, getHeight() - juce::roundToInt(25 * scale), getWidth(), juce::roundToInt(20 * scale));
    }

private:
    Track* track;
    ModernDialLookAndFeel globalLAF;

    MixerMeter peakMeter_1;
    juce::Slider midSideKnob;
    juce::Slider volumeSlider;
    juce::Slider panKnobNormal;
    DualPanBar dualPanBar;
    juce::TextButton reversePolarityBtn;
    juce::TextButton swapLRBtn;
    juce::TextButton bypassEffectsBtn;
    juce::TextButton muteSoloBtn;
    juce::Label trackName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelUI)
};

// ==============================================================================
// 3. CONTENEDOR PRINCIPAL DEL MIXER 
// ==============================================================================
class MixerComponent : public juce::Component, private juce::Timer {
public:
    MixerComponent();
    ~MixerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateChannels();
    void timerCallback() override;

    void setTracksReference(const juce::OwnedArray<Track>* tracks) {
        tracksRef = tracks;
        updateChannels();
    }

    float getMasterVolume() const { return masterVolume; }

private:
    const juce::OwnedArray<Track>* tracksRef = nullptr;
    float masterVolume = 1.0f;

    juce::Viewport viewport;
    juce::Component contentComp;
    juce::OwnedArray<MixerChannelUI> channels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};