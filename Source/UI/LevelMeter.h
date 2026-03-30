#pragma once
#include <JuceHeader.h>
#include "Knobs/FloatingValueBox.h"

// ==============================================================================
// MEDIDOR DE NIVEL ESTÉREO PRINCIPAL
// ==============================================================================
// AÑADIDO: public juce::SettableTooltipClient para que acepte la función setTooltip()
class LevelMeter : public juce::Component, public juce::SettableTooltipClient {
public:
    LevelMeter() {
        setOpaque(false);
    }

    ~LevelMeter() override {
        valueBox.removeFromDesktop();
    }

    void setLevel(float left, float right) {
        if (left >= 1.0f || right >= 1.0f) isClipping = true;

        if (left > absoluteMaxPeakL) absoluteMaxPeakL = left;
        if (right > absoluteMaxPeakR) absoluteMaxPeakR = right;

        dispL = left > dispL ? left : juce::jmax(0.0f, dispL - 0.04f);
        dispR = right > dispR ? right : juce::jmax(0.0f, dispR - 0.04f);

        if (left >= peakL) { peakL = left; holdL = 30; }
        else if (holdL > 0) { holdL--; }
        else { peakL = juce::jmax(0.0f, peakL - 0.01f); }

        if (right >= peakR) { peakR = right; holdR = 30; }
        else if (holdR > 0) { holdR--; }
        else { peakR = juce::jmax(0.0f, peakR - 0.01f); }

        if (isMouseOverMeter) updateValueBoxText();

        repaint();
    }

    void setHorizontal(bool horizontal) { isHorizontal = horizontal; repaint(); }

    void mouseEnter(const juce::MouseEvent&) override {
        isMouseOverMeter = true;
        updateValueBoxText();

        valueBox.addToDesktop(juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowIgnoresMouseClicks);
        valueBox.setSize(48, 20);
        positionValueBox();
        valueBox.setVisible(true);
    }

    void mouseMove(const juce::MouseEvent&) override {
        // Estático: no sigue al ratón
    }

    void mouseExit(const juce::MouseEvent&) override {
        isMouseOverMeter = false;
        valueBox.removeFromDesktop();
    }

    void mouseDown(const juce::MouseEvent&) override {
        absoluteMaxPeakL = 0.0f;
        absoluteMaxPeakR = 0.0f;
        isClipping = false;
        updateValueBoxText();
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto bounds = getLocalBounds().toFloat();
        
        if (!isHorizontal) {
            auto clipArea = bounds.removeFromTop(4.0f);
            g.setColour(isClipping ? juce::Colours::red : juce::Colour(40, 42, 45));
            g.fillRect(clipArea.reduced(0.0f, 0.5f));

            bounds.removeFromTop(1.0f);
            g.setColour(juce::Colour(15, 17, 20));
            g.fillRect(bounds);

            float h = bounds.getHeight();
            auto drawBar = [&](float currentVol, float maxPeak, juce::Rectangle<float> area) {
                float db = juce::Decibels::gainToDecibels(currentVol, -60.0f);
                float prop = juce::jlimit(0.0f, 1.0f, juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f));
                float peakDb = juce::Decibels::gainToDecibels(maxPeak, -60.0f);
                float peakProp = juce::jlimit(0.0f, 1.0f, juce::jmap(peakDb, -60.0f, 0.0f, 0.0f, 1.0f));
                int barY = area.getBottom() - (int)(prop * h);
                int barHeight = (int)(prop * h);
                juce::ColourGradient gradient(juce::Colours::limegreen, area.getBottomLeft(),
                    juce::Colours::red, area.getTopLeft(), false);
                gradient.addColour(0.8, juce::Colours::yellow);
                g.setGradientFill(gradient);
                if (barHeight > 0) g.fillRect(area.getX(), (float)barY, area.getWidth(), (float)barHeight);
                if (peakProp > 0.0f) {
                    int py = area.getBottom() - (int)(peakProp * h);
                    g.setColour(juce::Colours::white);
                    g.fillRect(area.getX(), (float)py, area.getWidth(), 1.0f);
                }
            };
            float halfW = bounds.getWidth() / 2.0f;
            drawBar(dispL, peakL, bounds.withWidth(halfW - 0.5f));
            drawBar(dispR, peakR, bounds.withTrimmedLeft(halfW + 0.5f));
        } else {
            auto clipArea = bounds.removeFromRight(4.0f);
            g.setColour(isClipping ? juce::Colours::red : juce::Colour(40, 42, 45));
            g.fillRect(clipArea.reduced(0.5f, 0.0f));

            bounds.removeFromRight(1.0f);
            g.setColour(juce::Colour(15, 17, 20));
            g.fillRect(bounds);

            float w = bounds.getWidth();
            auto drawHorizontalBar = [&](float currentVol, float maxPeak, juce::Rectangle<float> area) {
                float db = juce::Decibels::gainToDecibels(currentVol, -60.0f);
                float prop = juce::jlimit(0.0f, 1.0f, juce::jmap(db, -60.0f, 0.0f, 0.0f, 1.0f));
                float peakDb = juce::Decibels::gainToDecibels(maxPeak, -60.0f);
                float peakProp = juce::jlimit(0.0f, 1.0f, juce::jmap(peakDb, -60.0f, 0.0f, 0.0f, 1.0f));
                int barWidth = (int)(prop * w);
                
                juce::ColourGradient gradient(juce::Colours::limegreen, area.getTopLeft(),
                    juce::Colours::red, area.getTopRight(), false);
                gradient.addColour(0.8, juce::Colours::yellow);
                g.setGradientFill(gradient);
                if (barWidth > 0) g.fillRect(area.getX(), area.getY(), (float)barWidth, area.getHeight());
                if (peakProp > 0.0f) {
                    int px = area.getX() + (int)(peakProp * w);
                    g.setColour(juce::Colours::white);
                    g.fillRect((float)px, area.getY(), 1.0f, area.getHeight());
                }
            };
            float halfH = bounds.getHeight() / 2.0f;
            drawHorizontalBar(dispL, peakL, bounds.withHeight(halfH - 0.5f));
            drawHorizontalBar(dispR, peakR, bounds.withTrimmedTop(halfH + 0.5f));
        }
    }

private:
    void updateValueBoxText() {
        float maxAbsPeak = juce::jmax(absoluteMaxPeakL, absoluteMaxPeakR);
        float maxAbsDb = juce::Decibels::gainToDecibels(maxAbsPeak, -100.0f);

        juce::String textToShow;
        if (maxAbsDb <= -100.0f) {
            textToShow = "-inf";
        }
        else {
            juce::String prefix = maxAbsDb > 0.0f ? "+" : "";
            textToShow = prefix + juce::String(maxAbsDb, 1);
        }

        valueBox.setText(textToShow);
        valueBox.setBorderColor(isClipping ? juce::Colours::red : juce::Colour(200, 130, 30));
    }

    void positionValueBox() {
        int w = 48;
        int h = 20;
        auto screenPos = getScreenPosition();

        int x = screenPos.getX() - w - 8;
        int y = screenPos.getY() + (getHeight() / 2) - (h / 2);

        valueBox.setTopLeftPosition(x, y);
    }

    float dispL = 0.0f, dispR = 0.0f;
    float peakL = 0.0f, peakR = 0.0f;
    int holdL = 0, holdR = 0;

    bool isClipping = false;
    bool isMouseOverMeter = false;
    float absoluteMaxPeakL = 0.0f;
    float absoluteMaxPeakR = 0.0f;
    bool isHorizontal = false;

    FloatingValueBox valueBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};