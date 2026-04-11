#pragma once
#include <JuceHeader.h>
#include "../../../Theme/CustomTheme.h"
#include "../../../NativePlugins/VUMeter/VUBallistics.h"


class VUMeterComponent : public juce::Component, public juce::Timer
{
public:
    VUMeterComponent()
    {
        startTimerHz(60); 

        addAndMakeVisible(calibrationSlider);
        calibrationSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        calibrationSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        calibrationSlider.setRange(-24.0, -8.0, 1.0);
        calibrationSlider.setValue(-18.0);
        calibrationSlider.onValueChange = [this] { ballistics.setCalibration((float)calibrationSlider.getValue()); };
        calibrationSlider.setTooltip("Calibración: 0 VU = X dBFS");
    }
    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        auto h = area.getHeight() / 2;

        if (cachedDialL.isNull() || cachedDialL.getWidth() != area.getWidth() || cachedDialL.getHeight() != h)
            updateDials(area.getWidth(), h);

        // Renderizar Canal L
        g.drawImageAt(cachedDialL, 0, 0);
        drawChannel(g, juce::Rectangle<int>(0, 0, area.getWidth(), h).toFloat(), 
                    ballistics.getVuValueL(), ballistics.getPeakHoldL(), "L");

        // Renderizar Canal R
        g.drawImageAt(cachedDialR, 0, h);
        drawChannel(g, juce::Rectangle<int>(0, h, area.getWidth(), h).toFloat(), 
                    ballistics.getVuValueR(), ballistics.getPeakHoldR(), "R");

        calibrationSlider.setVisible(true);
    }

    void resized() override
    {
        auto area = getLocalBounds();
        calibrationSlider.setBounds(area.getRight() - 25, area.getBottom() - 25, 20, 20);
    }

    void timerCallback() override
    {
        repaint();
    }

    VUBallistics& getEngine() { return ballistics; }

private:
    VUBallistics ballistics;
    juce::Slider calibrationSlider;
    juce::Image cachedDialL, cachedDialR;

    struct Geometry {
        float centerX, pivotY, radius;
        float startAngle, endAngle;
    };

    Geometry calculateGeometry(float w, float h)
    {
        Geometry geo;
        geo.centerX = w * 0.5f;
        float topY = h * 0.2f;
        float edgeX = w * 0.05f;
        float edgeY = h * 0.8f;

        float dX = edgeX - geo.centerX;
        float dY = edgeY - topY;

        geo.radius = (dX * dX + dY * dY) / (2.0f * dY);
        geo.pivotY = topY + geo.radius;

        float angleAtEdge = std::atan2(geo.centerX - edgeX, geo.pivotY - edgeY);
        geo.startAngle = -angleAtEdge;
        geo.endAngle = angleAtEdge;

        return geo;
    }

    void updateDials(int w, int h)
    {
        cachedDialL = renderDialBackground(w, h, "L");
        cachedDialR = renderDialBackground(w, h, "R");
    }

    juce::Image renderDialBackground(int w, int h, juce::String label)
    {
        juce::Image img(juce::Image::ARGB, w, h, true);
        juce::Graphics g(img);
        auto area = juce::Rectangle<int>(w, h).toFloat();
        auto geo = calculateGeometry(area.getWidth(), area.getHeight());

        g.setColour(juce::Colour(15, 17, 20));
        g.fillRoundedRectangle(area.reduced(1.0f), 3.0f);
        
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.drawRoundedRectangle(area.reduced(1.0f), 3.0f, 1.5f);

        // Arco de Escala
        g.setColour(juce::Colours::white.withAlpha(0.25f));
        juce::Path arc;
        arc.addCentredArc(geo.centerX, geo.pivotY, geo.radius, geo.radius, 0.0f, 
                          geo.startAngle, geo.endAngle, true);
        g.strokePath(arc, juce::PathStrokeType(1.8f));

        // Marcas (Ticks)
        float ticks[] = { -20, -10, -7, -5, -3, -2, -1, 0, 1, 3, 5 };
        for (float v : ticks)
        {
            float angle = mapVUToAngle(v, geo);
            float innerR = geo.radius;
            float outerR = geo.radius + (v >= 0 ? 8.0f : 5.0f);

            g.setColour(v >= 0 ? juce::Colour::fromRGB(200, 40, 40) : juce::Colours::white.withAlpha(0.4f));
            g.drawLine(geo.centerX + std::sin(angle) * innerR, geo.pivotY - std::cos(angle) * innerR,
                       geo.centerX + std::sin(angle) * outerR, geo.pivotY - std::cos(angle) * outerR,
                       v >= 0 ? 1.8f : 1.0f);

            if (v == -20 || v == -10 || v == 0 || v == 3 || v == 5)
            {
                g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 9.5f, juce::Font::bold));
                juce::String t = (v > 0 ? "+" : "") + juce::String((int)v);
                float tx = geo.centerX + std::sin(angle) * (outerR + 10);
                float ty = geo.pivotY - std::cos(angle) * (outerR + 10);
                g.setColour(v >= 0 ? juce::Colour::fromRGB(200, 50, 50) : juce::Colours::lightgrey);
                g.drawText(t, (int)tx - 15, (int)ty - 8, 30, 16, juce::Justification::centred);
            }
        }

        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.setFont(juce::Font(juce::Font::getDefaultSansSerifFontName(), 10.0f, juce::Font::bold));
        g.drawText(label, 8, 8, 20, 20, juce::Justification::centred);
        g.drawText("VU", area.getCentreX() - 15, area.getBottom() - 25, 30, 20, juce::Justification::centred);

        juce::ColourGradient glass(juce::Colours::white.withAlpha(0.05f), 0, 0,
                                   juce::Colours::transparentWhite, 0, area.getHeight(), false);
        g.setGradientFill(glass);
        g.fillRect(area);

        return img;
    }

    void drawChannel(juce::Graphics& g, juce::Rectangle<float> area, float vuVal, float peakVal, juce::String label)
    {
        auto geo = calculateGeometry(area.getWidth(), area.getHeight());
        
        // 1. Aguja de Pico (Peak Hold) - Color rojo translúcido
        float peakAngle = mapVUToAngle(peakVal, geo);
        juce::Path peakPath;
        peakPath.addRectangle(-1.25f, -geo.radius * 1.02f, 2.5f, geo.radius * 1.02f);
        g.setColour(juce::Colour::fromRGB(220, 50, 50).withAlpha(0.6f));
        g.fillPath(peakPath, juce::AffineTransform::rotation(peakAngle).translated(geo.centerX, area.getY() + geo.pivotY));

        // 2. Aguja Principal (VU) - Naranja brillante
        float vuAngle = mapVUToAngle(vuVal, geo);
        juce::Path needle;
        needle.addRectangle(-0.85f, -geo.radius * 1.05f, 1.7f, geo.radius * 1.05f);
        g.setColour(juce::Colour(255, 85, 0)); 
        g.fillPath(needle, juce::AffineTransform::rotation(vuAngle).translated(geo.centerX, area.getY() + geo.pivotY));

        // Perno Central
        g.setColour(juce::Colour(30, 32, 35));
        g.fillEllipse(geo.centerX - 4, area.getBottom() - 8, 8, 8);
    }

    float mapVUToAngle(float db, const Geometry& geo)
    {
        // Mapeo Logarítmico/Voltaje (como el hardware real) basado en tu código
        float clamped = juce::jlimit(-20.0f, 5.0f, db);
        float vMin = std::pow(10.0f, -20.0f / 20.0f);
        float vMax = std::pow(10.0f, 5.0f / 20.0f);
        float vol = std::pow(10.0f, clamped / 20.0f);
        float t = (vol - vMin) / (vMax - vMin);
        
        return geo.startAngle + t * (geo.endAngle - geo.startAngle);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VUMeterComponent)
};
