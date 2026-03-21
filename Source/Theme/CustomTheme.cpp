#include "CustomTheme.h"

CustomTheme::CustomTheme()
{
    setColour(juce::DocumentWindow::textColourId, juce::Colours::white);
    setColour(juce::DocumentWindow::backgroundColourId, juce::Colour(28, 30, 34));
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(28, 30, 34));
}

void CustomTheme::drawDocumentWindowTitleBar(juce::DocumentWindow& window, juce::Graphics& g,
    int w, int h, int titleSpaceX, int titleSpaceW,
    const juce::Image* icon, bool drawTitleTextOnLeft)
{
    g.fillAll(juce::Colour(20, 22, 25));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawHorizontalLine(h - 1, 0, (float)w);

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font("Sans-Serif", 15.0f, juce::Font::bold));
    g.drawText(window.getName(), 0, 0, w, h, juce::Justification::centred);
}

// Reemplazamos los botones oficiales por nuestros SilentWindowButtons
juce::Button* CustomTheme::createDocumentWindowButton(int buttonType)
{
    if (buttonType == juce::DocumentWindow::minimiseButton) {
        auto* btn = new SilentWindowButton("-");
        btn->setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 54));
        btn->setColour(juce::TextButton::textColourOffId, juce::Colours::gold);
        return btn;
    }
    else if (buttonType == juce::DocumentWindow::maximiseButton) {
        auto* btn = new SilentWindowButton(juce::String::fromUTF8("\xe2\x96\xa1"));
        btn->setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 54));
        btn->setColour(juce::TextButton::textColourOffId, juce::Colours::limegreen);
        return btn;
    }
    else if (buttonType == juce::DocumentWindow::closeButton) {
        auto* btn = new SilentWindowButton("x");
        btn->setColour(juce::TextButton::buttonColourId, juce::Colour(150, 40, 40));
        btn->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        return btn;
    }
    return nullptr;
}