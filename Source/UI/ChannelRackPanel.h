#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"

// ==============================================================================
// Clase interna para estilizar los knobs pequeños estilo FL Studio
// ==============================================================================
class FLKnobLookAndFeel : public juce::LookAndFeel_V4 {
public:
    FLKnobLookAndFeel() {
        setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(255, 180, 0)); // Color de relleno al girar
        setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(40, 42, 45)); // Fondo del knob
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
        float sliderPosProportion, float rotaryStartAngle, float rotaryEndAngle,
        juce::Slider& slider) override
    {
        auto outline = slider.findColour(juce::Slider::rotarySliderOutlineColourId);
        auto fill = slider.findColour(juce::Slider::rotarySliderFillColourId);

        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toX = bounds.getCentreX();
        auto toY = bounds.getCentreY();
        auto lineW = 3.0f;
        auto arcRadius = radius - lineW * 0.5f;

        // Dibujar fondo del círculo
        g.setColour(outline);
        g.fillEllipse(bounds.getCentreX() - radius, bounds.getCentreY() - radius, radius * 2.0f, radius * 2.0f);

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(toX, toY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(outline.darker(0.5f));
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (slider.isEnabled()) {
            // Dibujar arco de progreso
            juce::Path valueArc;
            valueArc.addCentredArc(toX, toY, arcRadius, arcRadius, 0.0f,
                rotaryStartAngle,
                rotaryStartAngle + sliderPosProportion * (rotaryEndAngle - rotaryStartAngle),
                true);

            g.setColour(fill);
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Dibujar la línea indicadora central
            g.setColour(juce::Colours::white.withAlpha(0.7f));
            g.drawLine(toX, toY,
                toX + (radius * 0.7f) * std::cos(rotaryStartAngle + sliderPosProportion * (rotaryEndAngle - rotaryStartAngle) - juce::MathConstants<float>::halfPi),
                toY + (radius * 0.7f) * std::sin(rotaryStartAngle + sliderPosProportion * (rotaryEndAngle - rotaryStartAngle) - juce::MathConstants<float>::halfPi),
                1.5f);
        }
    }
};

// ==============================================================================
// Componente que representa UNA SOLA FILA (pista) en el Channel Rack
// ==============================================================================
class ChannelRackRow : public juce::Component {
public:
    ChannelRackRow(Track& t) : track(t) {
        // --- BYPASS / MUTE (LED Verde) ---
        addAndMakeVisible(muteButton);
        muteButton.setClickingTogglesState(true);
        muteButton.setToggleState(true, juce::dontSendNotification); // Encendido por defecto
        muteButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::limegreen);
        muteButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
        muteButton.setTooltip("Bypass/Mute");

        // --- PANNING KNOB ---
        addAndMakeVisible(panSlider);
        panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panSlider.setLookAndFeel(&knobLookAndFeel);
        panSlider.setRange(-1.0, 1.0, 0.01);
        panSlider.setValue(0.0);
        panSlider.setTooltip("Paneo");

        // --- VOLUME/BALANCE KNOB ---
        addAndMakeVisible(volSlider);
        volSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        volSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        volSlider.setLookAndFeel(&knobLookAndFeel);
        volSlider.setRange(0.0, 1.0, 0.01);
        volSlider.setValue(0.8);
        volSlider.setTooltip("Volumen");

        // --- TRACK NAME (Botón estirado) ---
        addAndMakeVisible(nameLabel);
        nameLabel.setText(track.getName(), juce::dontSendNotification);
        nameLabel.setFont(juce::Font(13.0f));
        nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
        nameLabel.setJustificationType(juce::Justification::centredLeft);

        // --- 16 STEPS ---
        for (int i = 0; i < 16; ++i) {
            auto* step = stepButtons.add(new juce::ToggleButton());
            addAndMakeVisible(step);
            step->setClickingTogglesState(true);
            step->setTooltip("Paso " + juce::String(i + 1));

            // Colores estilo FL Studio (Gris claro para beats 1 y 3, Rojo/Gris oscuro para 2 y 4)
            if ((i / 4) % 2 == 0) { // Beats 1 y 3
                step->setColour(juce::ToggleButton::tickColourId, juce::Colour(200, 200, 200)); // Encendido
                step->setColour(juce::ToggleButton::textColourId, juce::Colour(60, 62, 65));    // Apagado
            }
            else { // Beats 2 y 4
                step->setColour(juce::ToggleButton::tickColourId, juce::Colours::indianred);    // Encendido
                step->setColour(juce::ToggleButton::textColourId, juce::Colour(45, 47, 50));    // Apagado
            }
        }
    }

    void paint(juce::Graphics& g) override {
        // Dibujar el fondo del nombre estilo botón redondeado
        juce::Rectangle<int> nameBounds(68, 4, 90, getHeight() - 8);
        g.setColour(track.getColor().withAlpha(0.15f));
        g.fillRoundedRectangle(nameBounds.toFloat(), 4.0f);
        g.setColour(track.getColor().withAlpha(0.4f));
        g.drawRoundedRectangle(nameBounds.toFloat(), 4.0f, 1.0f);
    }

    void resized() override {
        auto area = getLocalBounds();

        // Espaciado inicial
        area.removeFromLeft(5);

        // 1. Bypass (Mute)
        muteButton.setBounds(area.removeFromLeft(20).reduced(0, 8));
        area.removeFromLeft(3);

        // 2. Pan Knob
        panSlider.setBounds(area.removeFromLeft(20).reduced(0, 3));

        // 3. Vol Knob
        volSlider.setBounds(area.removeFromLeft(20).reduced(0, 3));
        area.removeFromLeft(5);

        // 4. Nombre (Ocupa espacio fijo)
        nameLabel.setBounds(area.removeFromLeft(90).reduced(5, 0));
        area.removeFromLeft(10); // Espacio antes de los pasos

        // 5. Steps (Ocupan TODO el espacio horizontal restante)
        int numSteps = stepButtons.size();
        float stepW = (float)area.getWidth() / numSteps;
        float currentX = (float)area.getX();

        for (auto* step : stepButtons) {
            step->setBounds((int)currentX, area.getY() + 4, (int)stepW - 2, area.getHeight() - 8);
            currentX += stepW;
        }
    }

private:
    Track& track;
    juce::ToggleButton muteButton;
    juce::Slider panSlider, volSlider;
    juce::Label nameLabel;
    juce::OwnedArray<juce::ToggleButton> stepButtons;
    FLKnobLookAndFeel knobLookAndFeel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackRow)
};

// ==============================================================================
// Componente PRINCIPAL del Channel Rack (Contenedor con Scroll)
// ==============================================================================
class ChannelRackPanel : public juce::Component, private juce::Timer {
public:
    ChannelRackPanel() {
        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&contentComponent, false);
        viewport.setScrollBarsShown(true, false, true, false);

        startTimerHz(2);
    }

    void setTrackContainer(TrackContainer* tc) {
        trackContainer = tc;
        updateRows();
    }

    void updateRows() {
        if (!trackContainer) return;

        juce::Array<Track*> audioTracks;
        for (auto* t : trackContainer->getTracks()) {
            if (t->getType() == TrackType::Audio) audioTracks.add(t);
        }

        if (audioTracks.size() != currentAudioTrackCount) {
            // CORREGIDO: vaciar el array "rows" destruye los componentes y los quita de contentComponent automáticamente.
            rows.clear();
            currentAudioTrackCount = audioTracks.size();

            for (auto* t : audioTracks) {
                auto* newRow = rows.add(new ChannelRackRow(*t));
                contentComponent.addAndMakeVisible(newRow);
            }
            resized();
        }
    }

    void timerCallback() override { updateRows(); }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(30, 32, 35));

        if (!trackContainer || currentAudioTrackCount == 0) {
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.setFont(juce::Font(15.0f, juce::Font::italic));
            g.drawText("Añade Pistas de Audio (Sampler) para secuenciar...", getLocalBounds(), juce::Justification::centred);
        }
    }

    void resized() override {
        viewport.setBounds(getLocalBounds());

        int rowHeight = 35;
        int totalHeight = currentAudioTrackCount * rowHeight + 20;

        int contentW = viewport.getWidth() - (viewport.isVerticalScrollBarShown() ? viewport.getScrollBarThickness() : 0);
        contentComponent.setBounds(0, 0, contentW, totalHeight);

        for (int i = 0; i < rows.size(); ++i) {
            rows[i]->setBounds(0, i * rowHeight + 10, contentComponent.getWidth(), rowHeight);
        }
    }

private:
    TrackContainer* trackContainer = nullptr;
    int currentAudioTrackCount = 0;

    juce::Viewport viewport;
    juce::Component contentComponent;
    juce::OwnedArray<ChannelRackRow> rows;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelRackPanel)
};