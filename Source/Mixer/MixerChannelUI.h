#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"

// ==============================================================================
// 1. LOOK AND FEEL PROFESIONAL PARA EL MIXER
// ==============================================================================
class MixerLookAndFeel : public juce::LookAndFeel_V4 {
public:
    MixerLookAndFeel() {
        setColour(juce::Slider::thumbColourId, juce::Colour(200, 200, 200));
        setColour(juce::Slider::trackColourId, juce::Colour(30, 30, 30));
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
        const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) override {
        auto outline = juce::Colour(50, 50, 50);
        auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toX = bounds.getCentreX();
        auto toY = bounds.getCentreY();
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        // Fondo del dial
        g.setColour(juce::Colour(25, 25, 25));
        g.fillEllipse(toX - radius, toY - radius, radius * 2.0f, radius * 2.0f);
        
        // Borde sutil
        g.setColour(outline);
        g.drawEllipse(toX - radius, toY - radius, radius * 2.0f, radius * 2.0f, 1.5f);

        // Indicador (Puntero)
        juce::Path p;
        auto pointerLength = radius * 0.8f;
        auto pointerThickness = 2.5f;
        p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(toX, toY));

        g.setColour(juce::Colours::orange.withAlpha(0.9f));
        g.fillPath(p);
    }

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPos, float minSliderPos, float maxSliderPos,
        const juce::Slider::SliderStyle style, juce::Slider& slider) override {
        
        auto trackWidth = 4.0f;
        auto isVertical = style == juce::Slider::LinearVertical;

        // Dibujar el rail/pista
        g.setColour(juce::Colour(15, 15, 15));
        if (isVertical)
            g.fillRoundedRectangle(x + width * 0.5f - trackWidth * 0.5f, y, trackWidth, height, 2.0f);
        else
            g.fillRoundedRectangle(x, y + height * 0.5f - trackWidth * 0.5f, width, trackWidth, 2.0f);

        // Dibujar el "fader handle" (el bloque deslizante)
        auto handleWidth = isVertical ? width * 0.8f : 30.0f;
        auto handleHeight = isVertical ? 20.0f : height * 0.8f;
        
        juce::Rectangle<float> handle;
        if (isVertical)
            handle = { x + width * 0.5f - handleWidth * 0.5f, sliderPos - handleHeight * 0.5f, handleWidth, handleHeight };
        else
            handle = { sliderPos - handleWidth * 0.5f, y + height * 0.5f - handleHeight * 0.5f, handleWidth, handleHeight };

        auto c = juce::Colour(60, 60, 65);
        g.setColour(c);
        g.fillRoundedRectangle(handle, 3.0f);
        
        // Línea central de color en el fader
        g.setColour(juce::Colours::orange);
        if (isVertical)
            g.fillRect(handle.getX(), handle.getCentreY() - 1.0f, handle.getWidth(), 2.0f);
        else
            g.fillRect(handle.getCentreX() - 1.0f, handle.getY(), 2.0f, handle.getHeight());
        
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawRoundedRectangle(handle, 3.0f, 1.0f);
    }
};

// ==============================================================================
// 2. VÚMETRO ESTÉREO AVANZADO
// ==============================================================================
class AdvancedMeter : public juce::Component, public juce::Timer {
public:
    AdvancedMeter(Track* t) : track(t) { startTimerHz(30); }
    void timerCallback() override { repaint(); }

    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().toFloat();
        g.setColour(juce::Colour(10, 10, 10));
        g.fillRoundedRectangle(b, 2.0f);

        if (!track) return;

        float l = track->currentPeakLevelL;
        float r = track->currentPeakLevelR;

        // Peak Hold logic
        if (l > maxL) { maxL = l; ticksSincePeakL = 0; } else { if (++ticksSincePeakL > 30) maxL *= 0.95f; }
        if (r > maxR) { maxR = r; ticksSincePeakR = 0; } else { if (++ticksSincePeakR > 30) maxR *= 0.95f; }

        drawBar(g, b.removeFromLeft(b.getWidth() * 0.48f), l, maxL);
        b.removeFromLeft(b.getWidth() * 0.04f); // Espaciador
        drawBar(g, b, r, maxR);
    }

private:
    void drawBar(juce::Graphics& g, juce::Rectangle<float> area, float level, float peak) {
        float h = area.getHeight();
        float levelY = juce::jmap(juce::Decibels::gainToDecibels(level), -60.0f, 6.0f, h, 0.0f);
        float peakY = juce::jmap(juce::Decibels::gainToDecibels(peak), -60.0f, 6.0f, h, 0.0f);
        
        levelY = juce::jlimit(0.0f, h, levelY);
        peakY = juce::jlimit(0.0f, h, peakY);

        // Gradiente de color profesional
        juce::ColourGradient grad(juce::Colours::red, area.getX(), area.getY(), juce::Colours::limegreen, area.getX(), area.getBottom(), false);
        grad.addColour(0.2, juce::Colours::yellow);
        g.setGradientFill(grad);
        
        g.fillRect(area.withTop(levelY));

        // Peak line
        g.setColour(juce::Colours::white);
        g.fillRect(area.getX(), peakY, area.getWidth(), 1.5f);
    }

    Track* track;
    float maxL = 0, maxR = 0;
    int ticksSincePeakL = 0, ticksSincePeakR = 0;
};

// ==============================================================================
// 3. SLOTS DE EFECTOS Y ENVÍOS
// ==============================================================================
class PluginSlot : public juce::Component {
public:
    PluginSlot(Track* t, int idx) : track(t), index(idx) {}
    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().reduced(1);
        bool hasPlugin = track && index < track->plugins.size();
        
        g.setColour(hasPlugin ? juce::Colour(45, 50, 60) : juce::Colour(25, 25, 25));
        g.fillRoundedRectangle(b.toFloat(), 2.0f);
        
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawRoundedRectangle(b.toFloat(), 2.0f, 1.0f);

        if (hasPlugin) {
            auto* p = track->plugins[index];
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            g.setFont(10.0f);
            g.drawText(p->getLoadedPluginName().substring(0, 12), b.reduced(2), juce::Justification::centredLeft);
        }
    }
private:
    Track* track;
    int index;
};

class SendSlot : public juce::Component {
public:
    SendSlot(Track* t, int idx) : track(t), index(idx) {}
    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().reduced(1);
        bool hasSend = track && index < track->sends.size();
        
        g.setColour(hasSend ? juce::Colour(35, 60, 45) : juce::Colour(20, 25, 20));
        g.fillRoundedRectangle(b.toFloat(), 2.0f);

        if (hasSend) {
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.setFont(9.0f);
            g.drawText("SEND " + juce::String(index + 1), b.reduced(2), juce::Justification::centred);
        }
    }
private:
    Track* track;
    int index;
};

// ==============================================================================
// 4. CANAL PRINCIPAL DEL MIXER (Overhaul Profesional)
// ==============================================================================
class MixerChannelUI : public juce::Component {
public:
    MixerChannelUI(Track* t) : track(t), meter(t) {
        setLookAndFeel(&mixerLAF);

        // --- FX Rack (10 slots) ---
        for (int i = 0; i < 10; ++i) {
            auto* s = new PluginSlot(track, i);
            fxSlots.add(s);
            addAndMakeVisible(s);
        }

        // --- Sends Rack (4 slots) ---
        for (int i = 0; i < 4; ++i) {
            auto* s = new SendSlot(track, i);
            sendSlots.add(s);
            addAndMakeVisible(s);
        }

        // --- Paneo ---
        panToggle.setButtonText("NORM");
        panToggle.setClickingTogglesState(true);
        panToggle.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkgrey);
        panToggle.onClick = [this] { 
            bool dual = panToggle.getToggleState();
            panToggle.setButtonText(dual ? "DUAL" : "NORM");
            track->panningModeDual.store(dual);
            updatePanVisibility();
        };
        addAndMakeVisible(panToggle);

        panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panKnob.setRange(-1.0, 1.0);
        panKnob.setValue(track->getBalance());
        panKnob.onValueChange = [this] { track->setBalance((float)panKnob.getValue()); };
        addAndMakeVisible(panKnob);

        panL.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panL.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panL.setRange(-1.0, 1.0);
        panL.setValue(track->panL.load());
        panL.onValueChange = [this] { track->panL.store((float)panL.getValue()); };
        addChildComponent(panL);

        panR.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panR.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panR.setRange(-1.0, 1.0);
        panR.setValue(track->panR.load());
        panR.onValueChange = [this] { track->panR.store((float)panR.getValue()); };
        addChildComponent(panR);

        // --- Ganancia / Plugins ---
        gainKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        gainKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        gainKnob.setRange(0.0, 2.0);
        gainKnob.setValue(1.0);
        addAndMakeVisible(gainKnob);

        // --- Fader y Meter ---
        addAndMakeVisible(meter);
        
        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        fader.setRange(0.0, 1.5);
        fader.setValue(track->getVolume());
        fader.onValueChange = [this] { track->setVolume((float)fader.getValue()); };
        addAndMakeVisible(fader);

        // --- Botones de Control ---
        setupButton(muteBtn, "M", juce::Colours::red);
        muteBtn.setToggleState(track->isMuted, juce::dontSendNotification);
        muteBtn.onClick = [this] { track->isMuted = muteBtn.getToggleState(); };
        
        setupButton(soloBtn, "S", juce::Colours::yellow);
        soloBtn.setToggleState(track->isSoloed, juce::dontSendNotification);
        soloBtn.onClick = [this] { track->isSoloed = soloBtn.getToggleState(); };

        setupButton(phaseBtn, "PHS", juce::Colours::cyan);
        phaseBtn.setToggleState(track->isPhaseInverted, juce::dontSendNotification);
        phaseBtn.onClick = [this] { track->isPhaseInverted = phaseBtn.getToggleState(); };

        setupButton(recBtn, "R", juce::Colours::red.brighter());

        // --- Info ---
        trackName.setText(track->getName(), juce::dontSendNotification);
        trackName.setJustificationType(juce::Justification::centred);
        trackName.setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(trackName);

        updatePanVisibility();
    }

    ~MixerChannelUI() override { setLookAndFeel(nullptr); }

    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds();
        g.fillAll(juce::Colour(35, 38, 42));
        
        // Separador lateral
        g.setColour(juce::Colours::black.withAlpha(0.3f));
        g.drawRect(b, 1);

        // Franja de color del track
        g.setColour(track->getColor());
        g.fillRect(0, getHeight() - 5, getWidth(), 5);
    }

    void resized() override {
        auto b = getLocalBounds().reduced(4);
        
        // 1. Paneo (Arriba)
        auto topArea = b.removeFromTop(100);
        auto panToggleArea = topArea.removeFromTop(20);
        panToggle.setBounds(panToggleArea.withSizeKeepingCentre(40, 18));
        
        if (track->panningModeDual.load()) {
            panL.setBounds(topArea.removeFromLeft(topArea.getWidth() / 2).reduced(5));
            panR.setBounds(topArea.reduced(5));
        } else {
            panKnob.setBounds(topArea.withSizeKeepingCentre(50, 50));
        }

        // 2. FX Rack (10 slots de 16px c/u = 160px)
        auto fxArea = b.removeFromTop(160);
        for (int i = 0; i < fxSlots.size(); ++i) {
            fxSlots[i]->setBounds(fxArea.removeFromTop(16).reduced(1));
        }
        b.removeFromTop(5); // Espacio

        // 3. Sends Rack (4 slots de 15px c/u = 60px)
        auto sendArea = b.removeFromTop(60);
        for (int i = 0; i < sendSlots.size(); ++i) {
            sendSlots[i]->setBounds(sendArea.removeFromTop(15).reduced(1));
        }
        b.removeFromTop(5); // Espacio

        // 4. Botones de Control (M, S, PHS, R)
        auto btnArea = b.removeFromTop(30);
        auto btnW = btnArea.getWidth() / 4;
        muteBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(2));
        soloBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(2));
        phaseBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(2));
        recBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(2));

        // 5. Meter y Fader (Abajo)
        trackName.setBounds(b.removeFromBottom(20));
        auto faderArea = b;
        meter.setBounds(faderArea.removeFromRight(22));
        faderArea.removeFromRight(5); // Gap
        fader.setBounds(faderArea);
    }

private:
    void setupButton(juce::TextButton& btn, juce::String text, juce::Colour onCol) {
        btn.setButtonText(text);
        btn.setClickingTogglesState(true);
        btn.setColour(juce::TextButton::buttonOnColourId, onCol);
        btn.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        addAndMakeVisible(btn);
    }

    void updatePanVisibility() {
        bool dual = track->panningModeDual.load();
        panKnob.setVisible(!dual);
        panL.setVisible(dual);
        panR.setVisible(dual);
        resized();
    }

    Track* track;
    MixerLookAndFeel mixerLAF;
    
    // UI Elements
    juce::TextButton panToggle;
    juce::Slider panKnob, panL, panR;
    juce::Slider gainKnob;
    
    juce::OwnedArray<PluginSlot> fxSlots;
    juce::OwnedArray<SendSlot> sendSlots;

    AdvancedMeter meter;
    juce::Slider fader;
    juce::TextButton muteBtn, soloBtn, phaseBtn, recBtn;
    juce::Label trackName;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelUI)
};