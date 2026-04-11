#pragma once
#include <JuceHeader.h>
#include "../../../Theme/CustomTheme.h"
#include "../../../NativePlugins/FFTAnalyzerPlugin/SimpleAnalyzer.h"

/**
 * InspectorPanel: Muestra el analisis de frecuencia promediado y en tiempo real
 * del track seleccionado, utilizando el motor FFT Analyzer integrado.
 * Restaurado para coincidir visualmente con el plugin original.
 */
class InspectorPanel : public juce::Component, public juce::Timer
{
public:
    InspectorPanel()
    {
        setOpaque(true);
        startTimerHz(60); 
        analyzer.setup(12, 44100.0);
        
        // CONFIGURACIÓN DE BOTONES (Restaurado)
        addAndMakeVisible(masterButton);
        masterButton.setButtonText("MASTER");
        masterButton.setRadioGroupId(1);
        masterButton.setClickingTogglesState(true);
        masterButton.setToggleState(true, juce::dontSendNotification);
        masterButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffeebb00));
        masterButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        masterButton.onClick = [this] { analyzer.setMasterMode(true); };

        addAndMakeVisible(defaultButton);
        defaultButton.setButtonText("DEFAULT");
        defaultButton.setRadioGroupId(1);
        defaultButton.setClickingTogglesState(true);
        defaultButton.setToggleState(false, juce::dontSendNotification);
        defaultButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff888888));
        defaultButton.onClick = [this] { analyzer.setMasterMode(false); };

        analyzer.setMasterMode(true); 
        analyzer.setCalibration(16.0f);
    }

    ~InspectorPanel() override
    {
        stopTimer();
    }

    void pushAudio(const float* data, int numSamples)
    {
        analyzer.pushBuffer(data, numSamples);
    }

    void setSampleRate(double sr)
    {
        if (sr > 0) analyzer.setup(12, sr);
    }

    void timerCallback() override
    {
        analyzer.process();
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        auto area = getLocalBounds();
        
        // Fondo Oscuro (Estilo SPAN/DAW)
        g.fillAll(juce::Colour(15, 15, 15));

        // --- DIBUJO DEL ANALIZADOR ---
        const float topM = 40, botM = 30, leftM = 45, rightM = 15;
        auto plotArea = area.toFloat().reduced(0);
        plotArea.setLeft(leftM);
        plotArea.setRight(area.getWidth() - rightM);
        plotArea.setTop(topM);
        plotArea.setBottom(area.getHeight() - botM);
        
        drawGrid(g, plotArea);
        
        auto& spectrumData = analyzer.getSpectrumData();
        auto& peakData = analyzer.getPeakData();

        if (spectrumData.size() > 1)
        {
            // Dibujar Espectro (Relleno)
            auto fillPath = createFillPath(spectrumData, plotArea);
            g.setColour(juce::Colour(0xff00ffbb).withAlpha(0.15f));
            g.fillPath(fillPath);
            
            // Dibujar Espectro (Linea superior)
            auto spectrumPath = createPath(spectrumData, plotArea);
            g.setColour(juce::Colour(0xff00ffbb));
            g.strokePath(spectrumPath, juce::PathStrokeType(1.2f));

            // Dibujar Picos (Linea Blanca Tenue)
            auto peakPath = createPath(peakData, plotArea);
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.strokePath(peakPath, juce::PathStrokeType(0.8f));
        }

        // --- CROSSHAIR Y INFO ---
        if (isMouseOverArea && mouseY > topM && mouseY < area.getHeight() - botM)
        {
            g.setColour(juce::Colours::white.withAlpha(0.35f)); // Mas opaco
            g.drawLine(mouseX, plotArea.getY(), mouseX, plotArea.getBottom(), 1.6f); // Mas grueso
            g.drawLine(plotArea.getX(), mouseY, plotArea.getRight(), mouseY, 1.6f);  // Mas grueso

            // Info de Frecuencia y Nivel
            float normX = (mouseX - plotArea.getX()) / plotArea.getWidth();
            float normY = (mouseY - plotArea.getY()) / plotArea.getHeight();
            
            float freq = 20.0f * std::pow(1000.0f, normX);
            float db = juce::jmap(1.0f - normY, -78.0f, -18.0f);

            juce::String infoText;
            infoText << (int)freq << " Hz   " << getNoteName(freq) << "   " << juce::String(db, 1) << " dB";
            
            g.setColour(juce::Colours::white);
            g.setFont(15.0f);
            g.drawText(infoText, 15, 10, 300, 20, juce::Justification::left);
        }
    }

    void resized() override 
    {
        int buttonWidth = 75;
        int buttonHeight = 22;
        int rightMargin = 15;
        int topMargin = 10;

        defaultButton.setBounds(getWidth() - buttonWidth - rightMargin, topMargin, buttonWidth, buttonHeight);
        masterButton.setBounds(getWidth() - (buttonWidth * 2) - rightMargin - 10, topMargin, buttonWidth, buttonHeight);
    }

    void mouseMove(const juce::MouseEvent& e) override
    {
        mouseX = (float)e.x;
        mouseY = (float)e.y;
        
        const float topM = 40, botM = 30, leftM = 45, rightM = 15;
        auto plotArea = getLocalBounds().toFloat();
        plotArea.setLeft(leftM);
        plotArea.setRight(getWidth() - rightM);
        plotArea.setTop(topM);
        plotArea.setBottom(getHeight() - botM);
        
        isMouseOverArea = plotArea.contains(mouseX, mouseY);
        repaint();
    }

    void mouseExit(const juce::MouseEvent& e) override
    {
        isMouseOverArea = false;
        repaint();
    }

    SimpleAnalyzer analyzer;

private:
    juce::TextButton masterButton, defaultButton;
    float mouseX = 0, mouseY = 0;
    bool isMouseOverArea = false;

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> area)
    {
        // --- LINEAS VERTICALES (FRECUENCIA LOG) ---
        // Dibujamos lineas en 20, 30, 40, 50, 60, 70, 80, 90, 100, 200...
        for (int i = 1; i <= 3; ++i) // 3 decadas: 20-200, 200-2k, 2k-20k
        {
            float decadeStart = 20.0f * std::pow(10.0f, (float)(i - 1));
            for (int j = 1; j <= 9; ++j)
            {
                float freq = decadeStart * j;
                if (freq > 20000.0f) break;

                float normX = std::log10(freq / 20.0f) / 3.0f; // log10(20000/20) = 3
                float x = area.getX() + normX * area.getWidth();

                // Linea mas fuerte para decadas, mas tenue para subs
                bool isMain = (j == 1 || j == 2 || j == 5);
                g.setColour(juce::Colours::white.withAlpha(isMain ? 0.08f : 0.03f));
                g.drawVerticalLine((int)x, area.getY(), area.getBottom());

                // Etiquetas en la parte inferior
                if (j == 1 || j == 2 || j == 3 || j == 4 || j == 6 || j == 8)
                {
                    if (freq >= 20.0f && freq <= 20000.0f)
                    {
                        g.setColour(juce::Colours::white.withAlpha(0.4f));
                        g.setFont(11.0f);
                        juce::String label;
                        if (freq >= 1000.0f) label = juce::String((int)(freq / 1000.0f)) + "k";
                        else label = juce::String((int)freq);
                        
                        // Solo dibujar si no se enciman demasiado
                        if (j == 1 || (j > 1 && freq < 1000) || (freq >= 1000 && j % 2 == 0))
                             g.drawText(label, (int)x - 20, (int)area.getBottom() + 5, 40, 20, juce::Justification::centred);
                    }
                }
            }
        }
        // Linea final 20k
        float x20k = area.getX() + (std::log10(20000.0f / 20.0f) / 3.0f) * area.getWidth();
        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.drawVerticalLine((int)x20k, area.getY(), area.getBottom());
        g.drawText("20k", (int)x20k - 20, (int)area.getBottom() + 5, 40, 20, juce::Justification::centred);

        // --- LINEAS HORIZONTALES (dB) ---
        for (float db = -18; db >= -78; db -= 6) // Cada 6dB para mas detalle
        {
            float normY = (db + 78.0f) / 60.0f;
            float y = area.getBottom() - normY * area.getHeight();
            
            bool isMain = ((int)db % 12 == 0);
            g.setColour(juce::Colours::white.withAlpha(isMain ? 0.08f : 0.03f));
            g.drawHorizontalLine((int)y, area.getX(), area.getRight());
            
            if (isMain)
            {
                g.setColour(juce::Colours::white.withAlpha(0.4f));
                g.setFont(10.0f);
                g.drawText(juce::String((int)db), 5, (int)y - 8, 35, 16, juce::Justification::right);
            }
        }
    }

    juce::Path createPath(const std::vector<float>& data, juce::Rectangle<float> area)
    {
        juce::Path p;
        if (data.empty()) return p;

        for (size_t i = 0; i < data.size(); ++i)
        {
            float x = area.getX() + (float)i / (float)(data.size() - 1) * area.getWidth();
            float val = juce::jlimit(-78.0f, -18.0f, data[i]);
            float normY = (val + 78.0f) / 60.0f;
            float y = area.getBottom() - normY * area.getHeight();

            if (i == 0) p.startNewSubPath(x, y);
            else p.lineTo(x, y);
        }
        
        return p;
    }

    juce::Path createFillPath(const std::vector<float>& data, juce::Rectangle<float> area)
    {
        juce::Path p = createPath(data, area);
        if (!p.isEmpty())
        {
            p.lineTo(area.getRight(), area.getBottom());
            p.lineTo(area.getX(), area.getBottom());
            p.closeSubPath();
        }
        return p;
    }

    juce::String getNoteName(float frequency)
    {
        if (frequency <= 0) return "";
        static const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        float midiNote = 12.0f * std::log2(frequency / 440.0f) + 69.0f;
        int n = (int)std::round(midiNote);
        if (n < 0 || n > 127) return "";
        return juce::String(noteNames[n % 12]) + juce::String(n / 12 - 1);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorPanel)
};
