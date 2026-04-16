#pragma once
#include <JuceHeader.h>
#include "FloatingValueBox.h"
#include "../../Engine/Modulation/ModulationTargets.h"
#include "../../Engine/Modulation/GridModulator.h"

// ==============================================================================
// KNOB QUE MUESTRA SU VALOR EN UNA CAJA FLOTANTE ESTÁTICA
// ==============================================================================
class FloatingValueSlider : public juce::Slider, private juce::Slider::Listener {
public:
    FloatingValueSlider() {
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addListener(this);
    }

    ~FloatingValueSlider() override {
        removeListener(this);
        valueBox.removeFromDesktop();
    }

    // Callback matemático para dar formato (dB, %, Pan)
    std::function<juce::String(double)> valueToTextFormattingCallback;

    void mouseEnter(const juce::MouseEvent& e) override {
        isHovering = true;

        if (showFloatingBox) {
            valueBox.addToDesktop(juce::ComponentPeer::windowIsTemporary |
                juce::ComponentPeer::windowIgnoresMouseClicks);
            valueBox.setSize(50, 20);
            positionValueBox(); // Lo posicionamos estático
            valueBox.setVisible(true);
        }

        juce::Slider::mouseEnter(e);
    }

    void mouseMove(const juce::MouseEvent& e) override {
        // YA NO SIGUE AL RATÓN (Eliminamos positionValueBox aquí)
        juce::Slider::mouseMove(e);
    }

    void mouseExit(const juce::MouseEvent& e) override {
        isHovering = false;
        if (!isDragging) valueBox.removeFromDesktop();
        juce::Slider::mouseExit(e);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        // --- LÓGICA DE LEARN (CORREGIDA) ---
        // Solo capturamos si hay un clic manual del usuario mientras el modo Learn está activo.
        if (GridModulator::pendingModulator != nullptr && modTarget.isValid()) {
            GridModulator::pendingModulator->addTarget(modTarget);
            // Ya NO devolvemos aquí; permitimos que el click siga al Slider para que no se "vuelva loco" o se bloquee.
        }

        isDragging = true;
        if (showFloatingBox) {
            valueBox.setBorderColor(juce::Colours::orange);
            positionValueBox();
        }
        juce::Slider::mouseDown(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        juce::Slider::mouseDrag(e);
    }

    void mouseUp(const juce::MouseEvent& e) override {
        isDragging = false;
        valueBox.setBorderColor(juce::Colour(200, 130, 30));
        if (!isHovering) valueBox.removeFromDesktop();
        juce::Slider::mouseUp(e);
    }

    void sliderValueChanged(juce::Slider*) override {
        updateValueBoxText();
    }

    void setModulationValue(float val) {
        getProperties().set("modValue", val);
        repaint();
    }

    ModTarget modTarget; // Identificador del parámetro para el sistema de modulación

    bool showFloatingBox = true; // Controlar si se muestra la caja flotante

private:
    void updateValueBoxText() {
        if (!showFloatingBox) return;
        if (valueToTextFormattingCallback) {
            valueBox.setText(valueToTextFormattingCallback(getValue()));
        }
        else {
            valueBox.setText(juce::String(getValue(), 1));
        }
    }

    void positionValueBox() {
        if (!showFloatingBox) return;
        updateValueBoxText();
        // ... rest of the function remains the same ...

        // Calculamos la posición del Slider en toda la pantalla
        auto screenPos = getScreenPosition();

        int w = 50;
        int h = 20;

        // Centrado horizontalmente respecto al Knob
        int x = screenPos.getX() + (getWidth() / 2) - (w / 2);
        // Aparece 5 píxeles justo ARRIBA del Knob
        int y = screenPos.getY() - h - 5;

        valueBox.setTopLeftPosition(x, y);
    }

    FloatingValueBox valueBox;
    bool isHovering = false;
    bool isDragging = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FloatingValueSlider)
};