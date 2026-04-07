#pragma once
#include <JuceHeader.h>
#include "PatternInlineEQ_DSP.h"

/**
 * @class PatternInlineEQ_GUI
 * Clase estática de utilidad para renderizar el EQ y el FFT sobre los clips MIDI.
 */
class PatternInlineEQ_GUI {
public:
    /**
     * Dibuja el analizador FFT de fondo (Sombra espectral)
     */
    static void drawFFTAnalysis(juce::Graphics& g, 
        const PatternInlineEQ_DSP& dsp, 
        juce::Rectangle<int> bounds, 
        juce::Colour trackColor)
    {
        const auto& magnitudes = dsp.getMagnitudes();
        if (magnitudes.empty()) return;

        juce::Path fftPath;
        const float width = (float)bounds.getWidth();
        const float height = (float)bounds.getHeight();
        const float bottom = (float)bounds.getBottom();

        fftPath.startNewSubPath((float)bounds.getX(), bottom);

        for (int i = 0; i < (int)magnitudes.size(); ++i)
        {
            float skew = 1.0f; 
            float xPercent = std::pow((float)i / (float)magnitudes.size(), 1.0f / 2.0f); // Mapeo Logarítmico simple
            float x = (float)bounds.getX() + xPercent * width;
            
            float mag = magnitudes[i];
            float y = bottom - (juce::jlimit(0.0f, 1.0f, mag * 2.0f) * height); // Ganancia visual boost x2

            if (i == 0) fftPath.startNewSubPath(x, y);
            else fftPath.lineTo(x, y);
        }

        fftPath.lineTo((float)bounds.getRight(), bottom);
        fftPath.closeSubPath();

        g.setColour(trackColor.withAlpha(0.15f));
        g.fillPath(fftPath);
        
        g.setColour(trackColor.withAlpha(0.3f));
        g.strokePath(fftPath, juce::PathStrokeType(1.0f));
    }

    /**
     * Dibuja la curva de respuesta del EQ (Overlay interactivo)
     */
    static void drawEQCurve(juce::Graphics& g, 
        const PatternInlineEQ_DSP& dsp, 
        juce::Rectangle<int> bounds, 
        juce::Colour trackColor)
    {
        const float width = (float)bounds.getWidth();
        const float height = (float)bounds.getHeight();
        const float midY = bounds.getCentreY();
        const float gainRange = 18.0f; // +-18dB

        // 1. DIBUJAR CURVA (Respuesta en magnitud aproximada)
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        juce::Path curvePath;
        
        for (int x = 0; x <= (int)width; x += 3)
        {
            float freq = xToFrequency((float)x, width);
            
            // Aproximación rápida para la Playlist:
            float totalGain = 0.0f;
            
            // Bell
            float bellDist = std::abs(std::log10(freq) - std::log10(dsp.getBellFreq()));
            if (bellDist < 0.3f) totalGain += dsp.getBellGain() * (1.0f - bellDist / 0.3f);
            
            // Shelfs
            if (freq < dsp.getLSFreq()) totalGain += dsp.getLSGain();
            if (freq > dsp.getHSFreq()) totalGain += dsp.getHSGain();
            
            // HP / LP (Visualización radical)
            if (freq < dsp.getHP()) totalGain -= 60.0f;
            if (freq > dsp.getLP()) totalGain -= 60.0f;

            float y = midY - (juce::jlimit(-18.0f, 18.0f, totalGain) / gainRange) * (height / 2.0f);
            
            if (x == 0) curvePath.startNewSubPath((float)bounds.getX() + x, y);
            else curvePath.lineTo((float)bounds.getX() + x, y);
        }
        g.strokePath(curvePath, juce::PathStrokeType(1.5f));

        // 2. DIBUJAR NODOS INTERACTIVOS (5 Puntos)
        struct NodeInfo { float f; float g; juce::Colour c; };
        std::vector<NodeInfo> nodes = {
            { dsp.getHP(), 0.0f, juce::Colours::red },
            { dsp.getLSFreq(), dsp.getLSGain(), juce::Colours::orange },
            { dsp.getBellFreq(), dsp.getBellGain(), juce::Colours::cyan },
            { dsp.getHSFreq(), dsp.getHSGain(), juce::Colours::yellow },
            { dsp.getLP(), 0.0f, juce::Colours::blue }
        };

        for (const auto& n : nodes)
        {
            float nx = (float)bounds.getX() + frequencyToX(n.f, width);
            float ny = midY - (n.g / gainRange) * (height / 2.0f);
            
            g.setColour(juce::Colours::white);
            g.fillEllipse(nx - 4.5f, ny - 4.5f, 9.0f, 9.0f);
            g.setColour(n.c.withAlpha(0.8f));
            g.drawEllipse(nx - 4.5f, ny - 4.5f, 9.0f, 9.0f, 1.5f);
        }
    }
    
    /**
     * Mapea una frecuencia (Hz) a una posición X (píxel) dentro del clip
     */
    static float frequencyToX(float frequency, float width)
    {
        float logMin = std::log10(20.0f);
        float logMax = std::log10(20000.0f);
        float logFreq = std::log10(juce::jlimit(20.0f, 20000.0f, frequency));
        return ((logFreq - logMin) / (logMax - logMin)) * width;
    }

    /**
     * Mapea una posición X (píxel) a una frecuencia (Hz)
     */
    static float xToFrequency(float x, float width)
    {
        float logMin = std::log10(20.0f);
        float logMax = std::log10(20000.0f);
        float logFreq = logMin + (x / width) * (logMax - logMin);
        return std::pow(10.0f, logFreq);
    }

    /**
     * Dibuja el espectro completo (FFT + Curva) en el clip
     */
    static void drawInlineFFT(juce::Graphics& g, juce::Rectangle<int> bounds, const PatternInlineEQ_DSP& dsp)
    {
        drawFFTAnalysis(g, dsp, bounds, juce::Colours::white);
        drawEQCurve(g, dsp, bounds, juce::Colours::white);
    }

    /**
     * Detecta si el ratón ha pulsado sobre un nodo de EQ
     */
    static int hitTestEQNodes(juce::Point<int> pos, juce::Rectangle<int> bounds, const PatternInlineEQ_DSP& dsp)
    {
        const float width = (float)bounds.getWidth();
        const float height = (float)bounds.getHeight();
        const float midY = (float)bounds.getCentreY();
        const float gainRange = 18.0f;

        struct NodeInfo { float f; float g; };
        std::vector<NodeInfo> nodes = {
            { dsp.getHP(), 0.0f },
            { dsp.getLSFreq(), dsp.getLSGain() },
            { dsp.getBellFreq(), dsp.getBellGain() },
            { dsp.getHSFreq(), dsp.getHSGain() },
            { dsp.getLP(), 0.0f }
        };

        for (int i = 0; i < (int)nodes.size(); ++i)
        {
            float nx = (float)bounds.getX() + frequencyToX(nodes[i].f, width);
            float ny = midY - (nodes[i].g / gainRange) * (height / 2.0f);
            
            if (pos.getDistanceSquaredFrom(juce::Point<float>(nx, ny).roundToInt()) < 100) // 10px radio
                return i;
        }

        return -1;
    }
};
