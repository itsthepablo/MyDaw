#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <algorithm>

// ==============================================================================
// 1. DSP: Balística VU y Peak Hold (Delay 0ms, Hold 2000ms)
// ==============================================================================
class VuMeterDSP
{
public:
    void prepare(double sampleRate) {
        fs = sampleRate;

        float tau = 0.3f / 4.6f;
        alpha = 1.0f - std::exp(-1.0f / (tau * fs));
        level = 0.0f;

        holdTimeSamples = (int)(fs * 2.0);
        holdCounter = 0;
        peakHoldLevel = 0.0f;

        peakDecay = std::pow(0.1f, 1.0f / fs);
    }

    void process(const float* buffer, int numSamples) {
        for (int i = 0; i < numSamples; ++i) {
            float absSample = std::abs(buffer[i]);

            level += (absSample - level) * alpha;

            if (level >= peakHoldLevel) {
                peakHoldLevel = level;
                holdCounter = holdTimeSamples;
            }
            else {
                if (holdCounter > 0) {
                    holdCounter--;
                }
                else {
                    peakHoldLevel *= peakDecay;
                }
            }
        }
    }

    void setCalibration(float refLevel) {
        calibrationOffset = refLevel;
    }

    float getVuValue() const {
        float dbfs = (level > 0.00001f) ? 20.0f * std::log10(level) : -100.0f;
        return dbfs + calibrationOffset;
    }

    float getPeakHoldVu() const {
        float dbfs = (peakHoldLevel > 0.00001f) ? 20.0f * std::log10(peakHoldLevel) : -100.0f;
        return dbfs + calibrationOffset;
    }

private:
    double fs = 44100.0;
    float alpha = 0.0f;
    float level = 0.0f;

    int holdTimeSamples = 0;
    int holdCounter = 0;
    float peakHoldLevel = 0.0f;
    float peakDecay = 0.99995f;

    float calibrationOffset = 5.0f;
};

// ==============================================================================
// 2. GUI: Ruteo y Dibujo de Interfaces
// ==============================================================================
class VuMeterGUI
{
public:
    static void draw(juce::Graphics& g, juce::Rectangle<int> bounds, float vuL, float vuR, float peakL, float peakR, float currentCalib, int viewMode)
    {
        if (viewMode == 0) drawClassic(g, bounds, vuL, vuR, peakL, peakR, currentCalib);
        else drawModern(g, bounds, vuL, vuR, peakL, peakR, currentCalib);
    }

private:
    // ==============================================================================
    // ESTILO 1: CLÁSICO (ARCO)
    // ==============================================================================
    static void drawClassic(juce::Graphics& g, juce::Rectangle<int> bounds, float vuL, float vuR, float peakL, float peakR, float currentCalib)
    {
        float vuVal = std::max(vuL, vuR);
        float peakVal = std::max(peakL, peakR);

        int meterW = bounds.getWidth() - 10;
        int meterH = (int)(meterW * 0.72f);

        juce::Rectangle<int> meterRect(0, 0, meterW, meterH);
        meterRect.setCentre(bounds.getCentre());

        g.setColour(juce::Colour::fromRGB(16, 16, 18));
        g.fillRoundedRectangle(meterRect.toFloat(), 6.0f);

        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(meterRect.toFloat().reduced(0.5f), 6.0f, 1.0f);
        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawRoundedRectangle(meterRect.toFloat().expanded(0.5f), 6.0f, 1.0f);

        g.setFont(9.0f);
        g.setColour(juce::Colours::grey.withAlpha(0.8f));
        juce::String refText = "REF: -" + juce::String(currentCalib, 1) + " dBFS";
        g.drawText(refText, meterRect.getX(), meterRect.getY() + 6, meterRect.getWidth(), 12, juce::Justification::centred);

        float cx = meterRect.getCentreX();
        float cy = meterRect.getBottom() - 12.0f;
        float radius = meterRect.getHeight() * 0.85f;
        float angleRange = juce::MathConstants<float>::pi * 0.33f;

        auto dbToAngle = [angleRange](float db) {
            float clamped = juce::jlimit(-20.0f, 3.0f, db);
            float vMin = std::pow(10.0f, -20.0f / 20.0f);
            float vMax = std::pow(10.0f, 3.0f / 20.0f);
            float vol = std::pow(10.0f, clamped / 20.0f);
            float t = (vol - vMin) / (vMax - vMin);
            return juce::jmap(t, -angleRange, angleRange);
            };

        float angle0 = dbToAngle(0.0f);
        float angle3 = dbToAngle(3.0f);
        float angleMinus20 = dbToAngle(-20.0f);

        juce::Path baseArc;
        baseArc.addCentredArc(cx, cy, radius, radius, 0.0f, angleMinus20, angle0, true);
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.strokePath(baseArc, juce::PathStrokeType(1.2f));

        juce::Path redArc;
        redArc.addCentredArc(cx, cy, radius, radius, 0.0f, angle0, angle3, true);
        g.setColour(juce::Colour::fromRGB(200, 40, 40));
        g.strokePath(redArc, juce::PathStrokeType(1.2f));

        juce::Path redBlock;
        redBlock.addCentredArc(cx, cy, radius - 4.0f, radius - 4.0f, 0.0f, angle0, angle3, true);
        g.setColour(juce::Colour::fromRGB(180, 30, 30).withAlpha(0.7f));
        g.strokePath(redBlock, juce::PathStrokeType(6.0f));

        float ticks[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 2, 3 };
        for (float v : ticks) {
            float tickAngle = dbToAngle(v);
            bool isRed = (v > 0);
            bool isMajor = (v == -20 || v == -10 || v == -5 || v == 0 || v == 3);

            float outerR = radius;
            float innerR = radius - (v == 0.0f ? 8.0f : (isMajor ? 6.0f : 4.0f));

            float x1 = cx + std::sin(tickAngle) * innerR;
            float y1 = cy - std::cos(tickAngle) * innerR;
            float x2 = cx + std::sin(tickAngle) * outerR;
            float y2 = cy - std::cos(tickAngle) * outerR;

            g.setColour(isRed ? juce::Colour::fromRGB(200, 40, 40) : juce::Colours::white.withAlpha(0.8f));
            g.drawLine(x1, y1, x2, y2, v == 0.0f ? 2.0f : 1.2f);

            float tx = cx + std::sin(tickAngle) * (radius - 18.0f);
            float ty = cy - std::cos(tickAngle) * (radius - 18.0f);

            g.setColour(isRed ? juce::Colour::fromRGB(200, 50, 50) : juce::Colours::lightgrey);
            g.setFont(juce::FontOptions(isMajor ? 10.0f : 8.5f, v == 0.0f ? juce::Font::bold : juce::Font::plain));

            juce::String text = (v > 0 ? "+" : "") + juce::String((int)v);
            g.drawText(text, (int)tx - 15, (int)ty - 8, 30, 16, juce::Justification::centred);
        }

        juce::Rectangle<int> vuBox((int)cx - 18, (int)(cy - meterH * 0.45f), 36, 18);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillRoundedRectangle(vuBox.toFloat(), 3.0f);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(vuBox.toFloat(), 3.0f, 1.0f);
        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        g.drawText("VU", vuBox, juce::Justification::centred);

        g.setFont(9.0f);
        g.setColour(juce::Colours::grey);
        g.drawText("CURRENT", meterRect.getX() + 6, meterRect.getBottom() - 32, 50, 10, juce::Justification::centredLeft);
        g.drawText("MAX", meterRect.getRight() - 56, meterRect.getBottom() - 32, 50, 10, juce::Justification::centredRight);

        auto drawLCD = [&](int x, int y, float val) {
            juce::Rectangle<int> lcd(x, y, 40, 16);
            g.setColour(juce::Colour::fromRGB(10, 10, 12));
            g.fillRoundedRectangle(lcd.toFloat(), 2.0f);
            g.setColour(juce::Colours::white.withAlpha(0.05f));
            g.drawRoundedRectangle(lcd.toFloat(), 2.0f, 1.0f);

            g.setColour(val > 0.0f ? juce::Colours::red : juce::Colours::white.withAlpha(0.9f));
            g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
            juce::String txt = (val <= -19.9f) ? "-Inf" : juce::String(val, 1);
            g.drawText(txt, lcd, juce::Justification::centred);
            };

        drawLCD(meterRect.getX() + 6, meterRect.getBottom() - 20, vuVal);
        drawLCD(meterRect.getRight() - 46, meterRect.getBottom() - 20, peakVal);

        float peakAngle = dbToAngle(peakVal);
        float px = cx + std::sin(peakAngle) * (radius + 2.0f);
        float py = cy - std::cos(peakAngle) * (radius + 2.0f);
        float pStartX = cx + std::sin(peakAngle) * (radius * 0.45f);
        float pStartY = cy - std::cos(peakAngle) * (radius * 0.45f);

        g.setColour(juce::Colour::fromRGB(220, 50, 50).withAlpha(0.9f));
        g.drawLine(pStartX, pStartY, px, py, 2.5f);

        float needleAngle = dbToAngle(vuVal);
        float nx = cx + std::sin(needleAngle) * (radius + 4.0f);
        float ny = cy - std::cos(needleAngle) * (radius + 4.0f);

        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawLine(cx + 2.0f, cy + 2.0f, nx + 2.0f, ny + 2.0f, 2.5f);

        g.setColour(juce::Colours::white);
        g.drawLine(cx, cy, nx, ny, 2.0f);

        float pivotRadius = 14.0f;
        juce::Path pivot;
        pivot.addCentredArc(cx, cy, pivotRadius, pivotRadius, 0.0f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
        pivot.closeSubPath();
        g.setColour(juce::Colour::fromRGB(20, 20, 22));
        g.fillPath(pivot);
        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.strokePath(pivot, juce::PathStrokeType(1.5f));

        g.setColour(juce::Colour::fromRGB(40, 40, 45));
        g.fillEllipse(cx - 3.0f, cy - 3.0f, 6.0f, 6.0f);
    }

    // ==============================================================================
    // ESTILO 2: MODERNO (BARRA HORIZONTAL)
    // ==============================================================================
    static void drawModern(juce::Graphics& g, juce::Rectangle<int> bounds, float vuL, float vuR, float peakL, float peakR, float currentCalib)
    {
        float vuVal = std::max(vuL, vuR);
        float peakVal = std::max(peakL, peakR);

        int meterW = bounds.getWidth() - 10;
        int meterH = (int)(meterW * 0.5f);

        juce::Rectangle<int> meterRect(0, 0, meterW, meterH);
        meterRect.setCentre(bounds.getCentre());

        g.setColour(juce::Colour::fromRGB(14, 14, 16));
        g.fillRoundedRectangle(meterRect.toFloat(), 4.0f);
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawRoundedRectangle(meterRect.toFloat().reduced(0.5f), 4.0f, 1.0f);

        // Referencia
        g.setFont(8.5f);
        g.setColour(juce::Colours::grey.withAlpha(0.6f));
        juce::String refText = "REF: -" + juce::String(currentCalib, 1) + " dBFS";
        g.drawText(refText, meterRect.getX(), meterRect.getY() + 4, meterRect.getWidth(), 12, juce::Justification::centred);

        float minDB = -50.0f; // Escala ampliada como en la imagen
        float maxDB = 5.0f;
        float startX = meterRect.getX() + 20.0f;
        float endX = meterRect.getRight() - 20.0f;
        float trackWidth = endX - startX;
        float trackY = meterRect.getCentreY() + 6.0f; // Centro de la barra horizontal

        auto dbToX = [&](float db) {
            float clamped = juce::jlimit(minDB, maxDB, db);
            float vMin = std::pow(10.0f, minDB / 20.0f);
            float vMax = std::pow(10.0f, maxDB / 20.0f);
            float vol = std::pow(10.0f, clamped / 20.0f);
            float t = (vol - vMin) / (vMax - vMin);
            return startX + t * trackWidth;
            };

        float x0 = dbToX(0.0f);
        float xMax = dbToX(maxDB);

        // --- 1. DIBUJO DE LAS BARRAS ---
        // Barra Superior Principal (Gris claro)
        g.setColour(juce::Colour::fromRGB(150, 150, 150));
        g.fillRect(startX, trackY, x0 - startX, 4.0f);
        // Barra Superior Roja
        g.setColour(juce::Colour::fromRGB(180, 20, 20));
        g.fillRect(x0, trackY, xMax - x0, 4.0f);

        // Barra Inferior (Gris Oscuro)
        float lowTrackY = trackY + 6.0f;
        g.setColour(juce::Colour::fromRGB(90, 90, 90));
        g.fillRect(startX, lowTrackY, x0 - startX, 2.0f);
        // Barra Inferior Roja
        g.setColour(juce::Colour::fromRGB(120, 20, 20));
        g.fillRect(x0, lowTrackY, xMax - x0, 2.0f);

        // --- 2. MARCAS Y NÚMEROS ---
        float ticks[] = { -50, -40, -30, -20, -10, -5, 0, 5 };
        for (float v : ticks) {
            float tx = dbToX(v);
            bool isRed = (v >= 0);

            g.setColour(isRed ? juce::Colour::fromRGB(180, 20, 20) : juce::Colours::white.withAlpha(0.6f));

            // Línea del tick (hacia arriba)
            g.drawLine(tx, trackY, tx, trackY - 6.0f, 1.5f);

            // Texto numérico
            g.setColour(isRed ? juce::Colour::fromRGB(220, 40, 40) : juce::Colours::lightgrey);
            g.setFont(juce::FontOptions(9.5f, v == 0.0f ? juce::Font::bold : juce::Font::plain));

            juce::String text = juce::String((int)v);
            if (v == -50) text = "-";
            if (v == 5) text = "5 +";
            if (v > 0 && v < 5) text = "+" + text;

            g.drawText(text, (int)tx - 15, (int)(trackY - 20.0f), 30, 12, juce::Justification::centred);
        }

        // Título Inferior
        g.setFont(8.5f);
        g.setColour(juce::Colours::grey.withAlpha(0.5f));
        g.drawText("VU LEVEL", meterRect.getX(), meterRect.getBottom() - 14, meterRect.getWidth(), 12, juce::Justification::centred);

        // --- 3. DIBUJO DE LAS AGUJAS ---
        auto drawNeedle = [&](float db, juce::Colour color, float thickness, bool isRedNeedle) {
            float nx = dbToX(db);

            // Aguja inclinada "/"
            float topX = nx + 12.0f;
            float topY = trackY - 20.0f;
            float botX = nx - 8.0f;
            float botY = trackY + 20.0f;

            if (isRedNeedle) {
                g.setColour(color);
                g.drawLine(botX, botY, topX, topY, thickness);
            }
            else {
                g.setColour(juce::Colours::black.withAlpha(0.8f));
                g.drawLine(botX + 2.0f, botY + 1.0f, topX + 2.0f, topY + 1.0f, thickness + 1.0f); // sombra fuerte
                g.setColour(color);
                g.drawLine(botX, botY, topX, topY, thickness);
            }
            };

        // Aguja Peak (Roja)
        drawNeedle(peakVal, juce::Colour::fromRGB(220, 50, 50).withAlpha(0.9f), 2.0f, true);

        // Aguja Principal (Blanca)
        drawNeedle(vuVal, juce::Colours::white, 2.0f, false);
    }
};