/*
  ==============================================================================
    HexColorPicker.h
    Selector de color estilo "Mapa de Color" con entrada Hexadecimal.
  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

class HexColorPicker : public juce::Component, public juce::ChangeListener, public juce::TextEditor::Listener
{
public:
    HexColorPicker(juce::Colour startColor, std::function<void(juce::Colour)> onColorChanged)
        : callback(onColorChanged),
        // AQUÍ ESTÁ EL TRUCO: 'showColourspace' muestra el cuadrado de gradiente grande
        selector(juce::ColourSelector::showColourspace)
    {
        selector.setCurrentColour(startColor);
        selector.addChangeListener(this);
        addAndMakeVisible(selector);

        // Etiqueta simple
        hexLabel.setText("HEX:", juce::dontSendNotification);
        hexLabel.setFont(14.0f);
        hexLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        addAndMakeVisible(hexLabel);

        // Caja de texto Hexadecimal
        // startColor.toString().substring(2) quita los dos primeros caracteres (FF) del Alpha
        hexInput.setText(startColor.toString().substring(2));
        hexInput.setJustification(juce::Justification::centred);
        hexInput.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha(0.3f));
        hexInput.setColour(juce::TextEditor::outlineColourId, juce::Colours::white.withAlpha(0.2f));
        hexInput.setColour(juce::TextEditor::textColourId, juce::Colours::cyan);
        hexInput.addListener(this);
        addAndMakeVisible(hexInput);

        setSize(240, 280); // Tamaño total de la ventana
    }

    ~HexColorPicker() override {
        selector.removeChangeListener(this);
        hexInput.removeListener(this);
    }

    void resized() override {
        auto area = getLocalBounds().reduced(8);

        // Reservamos espacio abajo para el HEX (30px)
        auto bottomRow = area.removeFromBottom(30);
        area.removeFromBottom(8); // Un pequeño margen de separación

        // Colocamos la etiqueta y el input
        hexLabel.setBounds(bottomRow.removeFromLeft(40));
        hexInput.setBounds(bottomRow);

        // El resto del espacio es para el Mapa de Color grande
        selector.setBounds(area);
    }

    void changeListenerCallback(juce::ChangeBroadcaster* source) override {
        if (source == &selector) {
            if (!isTyping) {
                auto c = selector.getCurrentColour();
                hexInput.setText(c.toString().substring(2), false);
                if (callback) callback(c);
            }
        }
    }

    void textEditorTextChanged(juce::TextEditor& editor) override {
        isTyping = true;
        juce::String text = editor.getText().trim().replace("#", "");

        // Validar si es un código de 6 dígitos válido
        if (text.length() == 6 && text.containsOnly("0123456789abcdefABCDEF")) {
            juce::Colour c = juce::Colour::fromString("FF" + text);
            selector.setCurrentColour(c);
            if (callback) callback(c);
        }
        isTyping = false;
    }

private:
    juce::ColourSelector selector;
    juce::Label hexLabel;
    juce::TextEditor hexInput;
    std::function<void(juce::Colour)> callback;
    bool isTyping = false;
};