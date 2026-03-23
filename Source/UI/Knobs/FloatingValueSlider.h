#pragma once
#include <JuceHeader.h>
#include "FloatingValueBox.h"

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

        valueBox.addToDesktop(juce::ComponentPeer::windowIsTemporary |
            juce::ComponentPeer::windowIgnoresMouseClicks);
        valueBox.setSize(50, 20);
        positionValueBox(); // Lo posicionamos estático
        valueBox.setVisible(true);

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
        isDragging = true;
        valueBox.setBorderColor(juce::Colours::orange); // Borde brillante al agarrar
        positionValueBox();
        juce::Slider::mouseDown(e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        // Tampoco sigue al ratón aquí. Solo cambia el valor visual (sliderValueChanged).
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

private:
    void updateValueBoxText() {
        if (valueToTextFormattingCallback) {
            valueBox.setText(valueToTextFormattingCallback(getValue()));
        }
        else {
            valueBox.setText(juce::String(getValue(), 1));
        }
    }

    void positionValueBox() {
        updateValueBoxText();

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