#include "HintPanel.h"

// ==============================================================================
// HINT PANEL IMPLEMENTATION
// ==============================================================================

HintPanel::HintPanel() {
    startTimerHz(30);
}

void HintPanel::paint(juce::Graphics& g) {
    // Fondo oscuro de la barra de estado
    g.fillAll(juce::Colour(20, 22, 25));

    // Línea sutil en la parte superior para separarla del Mixer/Playlist
    g.setColour(juce::Colours::black.withAlpha(0.8f));
    g.drawHorizontalLine(0, 0, (float)getWidth());
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawHorizontalLine(1, 0, (float)getWidth());

    if (currentHint.isEmpty()) return;

    // Icono ( i ) más limpio y espaciado
    juce::Rectangle<int> iconArea(15, getHeight() / 2 - 8, 16, 16);
    g.setColour(juce::Colour(120, 125, 130));
    g.drawEllipse(iconArea.toFloat(), 1.5f);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("i", iconArea, juce::Justification::centred, false);

    // Texto descriptivo con mucho espacio para respirar
    g.setColour(juce::Colours::white.withAlpha(0.9f));
    g.setFont(juce::Font(13.0f));

    // Dejamos margen para el icono y dibujamos el texto centrado a la izquierda
    g.drawText(currentHint, getLocalBounds().withTrimmedLeft(40).reduced(5, 0), juce::Justification::centredLeft, true);
}

void HintPanel::timerCallback() {
    juce::String newHint = "";

    auto* currentComp = juce::Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();

    if (currentComp != nullptr) {
        if (auto* tooltipClient = dynamic_cast<juce::TooltipClient*>(currentComp)) {
            newHint = tooltipClient->getTooltip();
        }

        if (newHint.isEmpty() && currentComp->getParentComponent() != nullptr) {
            if (auto* parentTooltipClient = dynamic_cast<juce::TooltipClient*>(currentComp->getParentComponent())) {
                newHint = parentTooltipClient->getTooltip();
            }
        }
    }

    if (currentHint != newHint) {
        currentHint = newHint;
        repaint();
    }
}
