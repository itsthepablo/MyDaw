#pragma once
#include <JuceHeader.h>
#include <vector>
#include "RenderEngine.h"

// MODELO DE LA TABLA ESTILO REAPER
class StatsTableModel : public juce::TableListBoxModel
{
public:
    int getNumRows() override { return 1; }
    void paintRowBackground(juce::Graphics& g, int, int, int, bool) override {
        g.fillAll(juce::Colour(45, 47, 50));
    }
    void paintCell(juce::Graphics& g, int rowNumber, int columnId,
                   int width, int height, bool rowIsSelected) override 
    {
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font("Consolas", 13.0f, juce::Font::plain));
        
        juce::String text;
        juce::Justification just = juce::Justification::centredRight;
        
        if (columnId == 1) { text = filename; just = juce::Justification::centredLeft; }
        else if (columnId == 2) { text = peak; }
        else if (columnId == 3) { text = clip; }
        else if (columnId == 4) { text = lufsm; }
        else if (columnId == 5) { text = lufss; }
        else if (columnId == 6) { text = lufsi; }
        else if (columnId == 7) { text = lra; }
        
        g.drawText(text, 5, 0, width - 10, height, just, true);
    }
    
    juce::String filename{""}, peak{""}, clip{""}, lufsm{""}, lufss{""}, lufsi{""}, lra{""};
};

class OfflineRenderer : public juce::Component, public juce::Timer
{
public:
    OfflineRenderer();
    ~OfflineRenderer() override;

    std::unique_ptr<RenderEngine> engine;
    std::function<void(juce::AudioBuffer<float>& buffer, int numSamples)> onProcessOfflineBlock;
    std::function<void(double sampleRate)> onPrepareEngine;
    std::function<void()> onClose;

    void showConfig(double totalLengthSecs);
    void startRenderUI();
    void cancelRenderUI();
    
    // --- NUEVA BATCH API (Stems / Multitrack) ---
    void setPendingStems(const std::vector<int>& stems);
    void startNextBatchStem();
    void resetBatchState();

    std::function<juce::String(int)> onGetTrackName;
    std::function<void(int)> onPrepareStemTrack;
    std::function<void()> onAllStemsFinished;
    
    bool isBatchProcessing = false;
    std::vector<int> pendingStemsQueue;
    int currentStemIndex = 0;

    void timerCallback() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    enum class ViewState { Configuration, Rendering };
    ViewState currentState = ViewState::Configuration;

    double storedTotalLength = 0.0;
    juce::File lastOutputFolder;
    juce::File lastOutputFile;

    // --- CONFIGURACIÓN UI (Ocultos en Rendering) ---
    juce::Label lblSource{ "", "Source:" };
    juce::ComboBox sourceCombo;
    juce::Label lblBounds{ "", "Bounds:" };
    juce::ComboBox boundsCombo;
    juce::ToggleButton tailToggle{ "Tail (ms)" }; 
    juce::TextEditor tailMsInput;
    juce::Label lblSampleRate{ "", "Sample rate:" };
    juce::ComboBox cbSampleRate;
    juce::Label lblParallel{ "", "Processing:" };
    juce::ToggleButton parallelRenderToggle{ "Parallel render" };
    juce::ToggleButton secondPassToggle{ "2nd pass render" };
    juce::Label lblBitDepth{ "", "WAV bit depth:" };
    juce::ComboBox cbBitDepth;
    juce::ToggleButton writeBwfMetadata{ "Write BWF chunk" };
    juce::Label lblLargeFiles{ "", "Large files:" };
    juce::ComboBox largeFilesCombo;
    juce::Label lblOutput{ "", "Directory:" };
    juce::TextEditor directoryInput;
    juce::Label lblFileName{ "", "File name:" };
    juce::TextEditor filenameInput;
    juce::ToggleButton skipSilentFiles{ "Skip silent files" };

    juce::TextButton btnStart{ "Render 1 file..." };
    juce::TextButton btnCloseConfig{ "Cancel" };

    // --- RENDERIZANDO UI (Estilo REAPER) ---
    juce::Label lblOutputTitle{ "", "Output file" };
    juce::TextEditor outputFileBox; // Readonly, bg oscuro, texto azulado
    juce::Label lblFormatInfo; // "WAV: 24bit PCM..."
    
    // Controles inferiores
    juce::TextButton btnOpenFolder{ "Open folder..." };
    juce::TextButton btnLaunchFile{ "Launch file" };
    juce::TextButton btnMediaExplorer{ "Media Explorer..." };
    juce::TextButton btnAddProject{ "Add file to project" };
    juce::TextButton btnStatsCharts{ "Stats/Charts" };
    juce::TextButton btnBack{ "Back" };
    juce::TextButton btnCloseRender{ "Close" };

    // Gráficos (Progress, VU, Waveform dibujados en paint)
    juce::Rectangle<int> progressRect;
    juce::Rectangle<int> vuRect;
    juce::Rectangle<int> waveRect;
    
    StatsTableModel tableModel;
    juce::TableListBox statsTable;

    std::vector<float> localWaveMax;
    std::vector<float> localWaveMin;
    uint32_t renderStartTimeMs = 0;
    
    juce::String currentInfoTimeStr;
    float currentProgress = 0.0f;
    float currentTruePeak = 0.0f;

    void updateVisibility();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OfflineRenderer)
};
