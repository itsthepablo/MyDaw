#pragma once
#include <JuceHeader.h>
#include "Knobs/FloatingValueBox.h"
#include "../Tracks/Track.h"

/**
 * LevelMeter — Componente Unificado Hi-Fi
 * Rediseñado con miembros al inicio para asegurar la resolución de nombres del compilador.
 */
class LevelMeter : public juce::Component, 
                   public juce::SettableTooltipClient,
                   private juce::Timer 
{
private:
    // --- MIEMBROS DE DATOS (Mover al inicio evita errores de resolución C++) ---
    Track* track = nullptr;
    float dispL = 0.0f, dispR = 0.0f;
    float peakL = 0.0f, peakR = 0.0f;
    int holdL = 0, holdR = 0;
    bool isClipping = false;
    bool isMouseOverMeter = false;
    bool isHorizontal = false;
    float absoluteMaxPeakL = 0.0f;
    float absoluteMaxPeakR = 0.0f;
    FloatingValueBox valueBox;

public:
    LevelMeter(Track* t = nullptr) : track(t) 
    {
        setOpaque(false);
        if (track != nullptr) startTimerHz(30);
    }

    ~LevelMeter() override 
    {
        stopTimer();
        valueBox.removeFromDesktop();
    }

    void setTrack(Track* t) 
    {
        track = t;
        if (track != nullptr) startTimerHz(30);
        else stopTimer();
    }

    // Interfaz manual
    void setLevel(float left, float right) 
    {
        if (left >= 1.0f || right >= 1.0f) isClipping = true;

        if (left > absoluteMaxPeakL) absoluteMaxPeakL = left;
        if (right > absoluteMaxPeakR) absoluteMaxPeakR = right;

        const float decayFactor = 0.94f; 
        dispL = left > dispL ? left : juce::jmax(0.0f, dispL * decayFactor);
        dispR = right > dispR ? right : juce::jmax(0.0f, dispR * decayFactor);

        auto updatePeak = [&](float current, float& peak, int& hold) {
            if (current >= peak) {
                peak = current;
                hold = 32;
            } else if (hold > 0) {
                hold--;
            } else {
                peak = juce::jmax(0.0f, peak * 0.98f);
            }
        };

        updatePeak(left, peakL, holdL);
        updatePeak(right, peakR, holdR);

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
        const float floorDB = -80.0f;

        if (!isHorizontal) {
            auto clipArea = bounds.removeFromTop(4.0f);
            g.setColour(isClipping ? juce::Colours::red : juce::Colour(35, 37, 40));
            g.fillRect(clipArea.reduced(0.5f, 0.5f));

            bounds.removeFromTop(2.0f);
            g.setColour(juce::Colour(10, 12, 14));
            g.fillRoundedRectangle(bounds, 2.0f);

            auto drawBar = [&](float currentVol, float maxPeak, juce::Rectangle<float> area) {
                float db = juce::Decibels::gainToDecibels(currentVol, floorDB);
                float prop = juce::jlimit(0.0f, 1.0f, juce::jmap(db, floorDB, 0.0f, 0.0f, 1.0f));
                
                float peakDb = juce::Decibels::gainToDecibels(maxPeak, floorDB);
                float peakProp = juce::jlimit(0.0f, 1.0f, juce::jmap(peakDb, floorDB, 0.0f, 0.0f, 1.0f));

                juce::ColourGradient gradient(juce::Colours::limegreen.withBrightness(0.6f), area.getBottomLeft(),
                                             juce::Colours::red, area.getTopLeft(), false);
                gradient.addColour(0.75, juce::Colours::orange);
                gradient.addColour(0.5, juce::Colours::limegreen);
                
                g.setGradientFill(gradient);
                float barHeight = prop * area.getHeight();
                if (barHeight > 0.5f) g.fillRect(area.withTop(area.getBottom() - barHeight));

                if (peakProp > 0.05f) {
                    float py = area.getBottom() - (peakProp * area.getHeight());
                    g.setColour(juce::Colours::white.withAlpha(0.6f));
                    g.fillRect(area.getX(), py, area.getWidth(), 1.0f);
                }
            };

            float halfW = bounds.getWidth() / 2.0f;
            drawBar(dispL, peakL, bounds.removeFromLeft(halfW).reduced(1.0f, 0.0f));
            drawBar(dispR, peakR, bounds.reduced(1.0f, 0.0f));
        } else {
            auto clipArea = bounds.removeFromRight(4.0f);
            g.setColour(isClipping ? juce::Colours::red : juce::Colour(35, 37, 40));
            g.fillRect(clipArea.reduced(0.5f, 0.5f));

            bounds.removeFromRight(2.0f);
            g.setColour(juce::Colour(10, 12, 14));
            g.fillRoundedRectangle(bounds, 2.0f);

            auto drawHorizontalBar = [&](float currentVol, float maxPeak, juce::Rectangle<float> area) {
                float db = juce::Decibels::gainToDecibels(currentVol, floorDB);
                float prop = juce::jlimit(0.0f, 1.0f, juce::jmap(db, floorDB, 0.0f, 0.0f, 1.0f));
                
                float peakDb = juce::Decibels::gainToDecibels(maxPeak, floorDB);
                float peakProp = juce::jlimit(0.0f, 1.0f, juce::jmap(peakDb, floorDB, 0.0f, 0.0f, 1.0f));

                juce::ColourGradient gradient(juce::Colours::limegreen.withBrightness(0.6f), area.getTopLeft(),
                                             juce::Colours::red, area.getTopRight(), false);
                gradient.addColour(0.75, juce::Colours::orange);
                g.setGradientFill(gradient);

                float barWidth = prop * area.getWidth();
                if (barWidth > 0.5f) g.fillRect(area.withWidth(barWidth));

                if (peakProp > 0.05f) {
                    float px = area.getX() + (peakProp * area.getWidth());
                    g.setColour(juce::Colours::white.withAlpha(0.6f));
                    g.fillRect(px, area.getY(), 1.0f, area.getHeight());
                }
            };

            float halfH = bounds.getHeight() / 2.0f;
            drawHorizontalBar(dispL, peakL, bounds.removeFromTop(halfH).reduced(0.0f, 1.0f));
            drawHorizontalBar(dispR, peakR, bounds.reduced(0.0f, 1.0f));
        }
    }

private:
    void timerCallback() override {
        if (track != nullptr) {
            float l = track->currentPeakLevelL.load(std::memory_order_relaxed);
            float r = track->currentPeakLevelR.load(std::memory_order_relaxed);
            setLevel(l, r);
        }
    }

    void updateValueBoxText() {
        float maxAbsPeak = juce::jmax(absoluteMaxPeakL, absoluteMaxPeakR);
        float maxAbsDb = juce::Decibels::gainToDecibels(maxAbsPeak, -100.0f);
        juce::String textToShow = (maxAbsDb <= -99.0f) ? "-inf" : ((maxAbsDb > 0.0f ? "+" : "") + juce::String(maxAbsDb, 1));
        valueBox.setText(textToShow);
        valueBox.setBorderColor(isClipping ? juce::Colours::red : juce::Colour(200, 130, 30));
    }

    void positionValueBox() {
        int w = 48, h = 20;
        auto screenPos = getScreenPosition();
        valueBox.setTopLeftPosition(screenPos.getX() - w - 8, screenPos.getY() + (getHeight() / 2) - (h / 2));
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LevelMeter)
};