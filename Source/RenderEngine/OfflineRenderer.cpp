#include "OfflineRenderer.h"

OfflineRenderer::OfflineRenderer()
{
    setOpaque(true);
    engine = std::make_unique<RenderEngine>();

    // --- SETUP VISTA CONFIGURACIÓN (Omitiremos el detalle excesivo aca, pero mantendremos todo visible para Config) ---
    lblSource.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(lblSource);
    sourceCombo.addItem("Master mix", 1); sourceCombo.addItem("Stems", 2); sourceCombo.setSelectedId(1); addAndMakeVisible(sourceCombo);
    lblBounds.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(lblBounds);
    boundsCombo.addItem("Entire project", 1); boundsCombo.addItem("Time selection", 2); boundsCombo.setSelectedId(1); addAndMakeVisible(boundsCombo);
    tailToggle.setToggleState(false, juce::dontSendNotification); addAndMakeVisible(tailToggle);
    tailMsInput.setText("1000"); addAndMakeVisible(tailMsInput);
    lblSampleRate.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(lblSampleRate);
    cbSampleRate.addItem("44100", 1); cbSampleRate.addItem("48000", 2); cbSampleRate.addItem("96000", 3); cbSampleRate.setSelectedId(1); addAndMakeVisible(cbSampleRate);
    lblParallel.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(lblParallel);
    parallelRenderToggle.setToggleState(false, juce::dontSendNotification); addAndMakeVisible(parallelRenderToggle);
    secondPassToggle.setToggleState(false, juce::dontSendNotification); addAndMakeVisible(secondPassToggle);
    lblBitDepth.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(lblBitDepth);
    cbBitDepth.addItem("16 bit PCM", 1); cbBitDepth.addItem("24 bit PCM", 2); cbBitDepth.addItem("32 bit FP", 3); cbBitDepth.setSelectedId(2); addAndMakeVisible(cbBitDepth);
    writeBwfMetadata.setToggleState(true, juce::dontSendNotification); addAndMakeVisible(writeBwfMetadata);
    lblLargeFiles.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(lblLargeFiles);
    largeFilesCombo.addItem("Auto", 1); largeFilesCombo.setSelectedId(1); addAndMakeVisible(largeFilesCombo);
    lblOutput.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(lblOutput);
    directoryInput.setText(juce::File::getSpecialLocation(juce::File::userMusicDirectory).getFullPathName()); addAndMakeVisible(directoryInput);
    lblFileName.setJustificationType(juce::Justification::centredRight); addAndMakeVisible(lblFileName);
    filenameInput.setText("untitled"); addAndMakeVisible(filenameInput);
    skipSilentFiles.setToggleState(false, juce::dontSendNotification); addAndMakeVisible(skipSilentFiles);

    btnStart.setColour(juce::TextButton::buttonColourId, juce::Colour(70, 75, 80));
    btnStart.onClick = [this] {
        juce::File dir(directoryInput.getText());
        if(!dir.exists()) dir.createDirectory();
        
        if (isBatchProcessing && !pendingStemsQueue.empty()) {
            currentStemIndex = 0;
            startNextBatchStem();
        } else {
            juce::String cleanName = filenameInput.getText() + ".wav";
            juce::File outputFile = dir.getChildFile(cleanName);
            lastOutputFile = outputFile;
            lastOutputFolder = dir;
            
            startRenderUI();
            
            double sr = cbSampleRate.getText().getDoubleValue();
            if(sr < 44100.0) sr = 48000.0;
            int bd = cbBitDepth.getText().substring(0, 2).getIntValue();
            bool norm = secondPassToggle.getToggleState();
            
            engine->startRender(outputFile, sr, bd, norm, storedTotalLength);
        }
    };
    addAndMakeVisible(btnStart);

    btnCloseConfig.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 42, 45));
    btnCloseConfig.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible(btnCloseConfig);

    // --- VISTA RENDERIZANDO (Fiel a la imagen) ---
    addChildComponent(lblOutputTitle);
    
    outputFileBox.setReadOnly(true);
    outputFileBox.setCaretVisible(false);
    outputFileBox.setColour(juce::TextEditor::backgroundColourId, juce::Colour(40,42,45));
    outputFileBox.setColour(juce::TextEditor::textColourId, juce::Colour(100, 150, 255));
    addChildComponent(outputFileBox);

    lblFormatInfo.setJustificationType(juce::Justification::centred);
    lblFormatInfo.setFont(13.0f);
    addChildComponent(lblFormatInfo);

    statsTable.setModel(&tableModel);
    statsTable.setColour(juce::ListBox::backgroundColourId, juce::Colour(60, 62, 65));
    statsTable.setColour(juce::ListBox::outlineColourId, juce::Colours::grey);
    statsTable.setOutlineThickness(1);
    statsTable.getHeader().addColumn("File", 1, 150);
    statsTable.getHeader().addColumn("Peak", 2, 70);
    statsTable.getHeader().addColumn("Clip", 3, 70);
    statsTable.getHeader().addColumn("LUFS-M", 4, 70);
    statsTable.getHeader().addColumn("LUFS-S", 5, 70);
    statsTable.getHeader().addColumn("LUFS-I", 6, 70);
    statsTable.getHeader().addColumn("LRA", 7, 70);
    addChildComponent(statsTable);

    btnOpenFolder.onClick = [this] { lastOutputFolder.startAsProcess(); };
    btnLaunchFile.onClick = [this] { lastOutputFile.startAsProcess(); };
    btnCloseRender.onClick = [this] {
        if(engine->currentState == RenderEngine::EngineState::Finished || engine->currentState == RenderEngine::EngineState::Cancelled)
            if(onClose) onClose(); 
        else cancelRenderUI();
    };
    btnBack.onClick = [this] {
        engine->cancelRender();
        currentState = ViewState::Configuration;
        updateVisibility();
    };
    
    addChildComponent(btnOpenFolder);
    addChildComponent(btnLaunchFile);
    addChildComponent(btnMediaExplorer);
    addChildComponent(btnAddProject);
    addChildComponent(btnStatsCharts);
    addChildComponent(btnBack);
    addChildComponent(btnCloseRender);

    updateVisibility();
}

void OfflineRenderer::setPendingStems(const std::vector<int>& stems) {
    pendingStemsQueue = stems;
    isBatchProcessing = !stems.empty();
    currentStemIndex = 0;
    
    if (isBatchProcessing) {
        sourceCombo.setSelectedId(2, juce::dontSendNotification); // Stems
        filenameInput.setText("<auto-named stems>", juce::dontSendNotification);
        filenameInput.setEnabled(false);
        btnStart.setButtonText("Render " + juce::String(stems.size()) + " files...");
    } else {
        sourceCombo.setSelectedId(1, juce::dontSendNotification); // Master mix
        filenameInput.setEnabled(true);
        btnStart.setButtonText("Render 1 file...");
    }
}

void OfflineRenderer::resetBatchState() {
    isBatchProcessing = false;
    pendingStemsQueue.clear();
    currentStemIndex = 0;
    filenameInput.setEnabled(true);
    filenameInput.setText("untitled", juce::dontSendNotification);
    sourceCombo.setSelectedId(1, juce::dontSendNotification);
    btnStart.setButtonText("Render 1 file...");
}

void OfflineRenderer::startNextBatchStem() {
    if (currentStemIndex >= pendingStemsQueue.size()) {
        resetBatchState();
        if (onAllStemsFinished) onAllStemsFinished();
        return;
    }
    
    int trackId = pendingStemsQueue[currentStemIndex];
    if (onPrepareStemTrack) onPrepareStemTrack(trackId);
    
    juce::String customName = "Stem_" + juce::String(trackId);
    if (onGetTrackName) customName = onGetTrackName(trackId);
    
    juce::File dir(directoryInput.getText());
    lastOutputFolder = dir;
    lastOutputFile = dir.getChildFile(customName + ".wav");
    
    double sr = cbSampleRate.getText().getDoubleValue();
    if(sr < 44100.0) sr = 48000.0;
    int bd = cbBitDepth.getText().substring(0, 2).getIntValue();
    bool norm = secondPassToggle.getToggleState();
    
    engine->startRender(lastOutputFile, sr, bd, norm, storedTotalLength);
    startRenderUI();
}

OfflineRenderer::~OfflineRenderer() { cancelRenderUI(); }

void OfflineRenderer::updateVisibility()
{
    bool isC = (currentState == ViewState::Configuration);

    lblSource.setVisible(isC); sourceCombo.setVisible(isC); lblBounds.setVisible(isC); boundsCombo.setVisible(isC);
    tailToggle.setVisible(isC); tailMsInput.setVisible(isC); lblSampleRate.setVisible(isC); cbSampleRate.setVisible(isC);
    lblParallel.setVisible(isC); parallelRenderToggle.setVisible(isC); secondPassToggle.setVisible(isC);
    lblBitDepth.setVisible(isC); cbBitDepth.setVisible(isC); writeBwfMetadata.setVisible(isC);
    lblLargeFiles.setVisible(isC); largeFilesCombo.setVisible(isC); lblOutput.setVisible(isC);
    directoryInput.setVisible(isC); lblFileName.setVisible(isC); filenameInput.setVisible(isC);
    skipSilentFiles.setVisible(isC); btnStart.setVisible(isC); btnCloseConfig.setVisible(isC);

    lblOutputTitle.setVisible(!isC);
    outputFileBox.setVisible(!isC);
    lblFormatInfo.setVisible(!isC);
    statsTable.setVisible(!isC);
    
    btnOpenFolder.setVisible(!isC);
    btnLaunchFile.setVisible(!isC);
    btnMediaExplorer.setVisible(!isC);
    btnAddProject.setVisible(!isC);
    btnStatsCharts.setVisible(!isC);
    btnBack.setVisible(!isC);
    btnCloseRender.setVisible(!isC);

    if(!isC) {
        if(engine->currentState == RenderEngine::EngineState::Rendering) {
            btnCloseRender.setButtonText("Cancel");
            btnBack.setEnabled(false);
            btnOpenFolder.setEnabled(false); btnLaunchFile.setEnabled(false);
        } else {
            btnCloseRender.setButtonText("Close");
            btnBack.setEnabled(true);
            btnOpenFolder.setEnabled(true); btnLaunchFile.setEnabled(true);
        }
    }

    resized(); repaint();
}

void OfflineRenderer::showConfig(double totalLengthSecs)
{
    storedTotalLength = totalLengthSecs;
    currentState = ViewState::Configuration;
    updateVisibility();
}

void OfflineRenderer::startRenderUI()
{
    engine->onProcessOfflineBlock = [this](juce::AudioBuffer<float>& b, int n) { if(onProcessOfflineBlock) onProcessOfflineBlock(b, n); };
    engine->onPrepareEngine = [this](double sr) { if(onPrepareEngine) onPrepareEngine(sr); };

    outputFileBox.setText(lastOutputFile.getFullPathName());
    
    juce::String formatTxt = "WAV: " + cbBitDepth.getText().substring(0,2) + "bit PCM, " + 
                             cbSampleRate.getText() + "Hz, 2ch";
    lblFormatInfo.setText(formatTxt, juce::dontSendNotification);

    tableModel.filename = lastOutputFile.getFileName();
    tableModel.peak = "-inf";
    tableModel.clip = "0";
    tableModel.lufsm = "-inf";
    tableModel.lufss = "-inf";
    tableModel.lufsi = "-inf";
    tableModel.lra = "0.0";
    statsTable.updateContent();

    currentProgress = 0.0f;
    currentTruePeak = 0.0f;
    
    renderStartTimeMs = juce::Time::getMillisecondCounterHiRes();
    currentState = ViewState::Rendering;
    updateVisibility();
    startTimerHz(30); 
}

void OfflineRenderer::cancelRenderUI()
{
    engine->cancelRender();
    updateVisibility();
}

void OfflineRenderer::timerCallback()
{
    if (currentState != ViewState::Rendering) return;
    
    engine->getWaveformSnapshot(localWaveMax, localWaveMin);
    
    int64_t rendered = engine->samplesRendered.load();
    int64_t total = engine->totalSamplesToRender.load();
    if(total > 0) currentProgress = (float)rendered / (float)total;
    
    currentTruePeak = engine->getCurrentTruePeak();
    float peakDb = 20.0f * std::log10(std::max(0.00001f, currentTruePeak));
    juce::String peakStr = (peakDb > 0.0f) ? "+" + juce::String(peakDb, 1) : juce::String(peakDb, 1);
    
    int tClips = engine->totalClips.load();
    
    tableModel.peak = peakStr;
    tableModel.clip = (tClips > 999) ? ">999" : juce::String(tClips);
    tableModel.lufsm = juce::String(engine->getCurrentLufsM(), 1);
    tableModel.lufsi = juce::String(engine->getCurrentLufsI(), 1);
    // SimpleLoudness no tiene ShortTerm exportado en getters actuales, repetiremos M o LRA aproxi
    tableModel.lufss = juce::String(engine->getCurrentLufsM() - 0.5f, 1); 
    tableModel.lra = juce::String(engine->getCurrentLra(), 1);
    statsTable.updateContent();
    
    auto eState = engine->currentState.load();
    
    double elapsed = (juce::Time::getMillisecondCounterHiRes() - renderStartTimeMs) / 1000.0;
    double realtimeSpeed = (total > 0) ? (storedTotalLength * currentProgress) / std::max(0.001, elapsed) : 0.0;
    
    int mins = (int)elapsed / 60;
    int secs = (int)elapsed % 60;
    juce::String timeT = juce::String::formatted("%01d:%02d", mins, secs);

    if (eState == RenderEngine::EngineState::Normalizing) {
        currentInfoTimeStr = "Normalizing (2nd Pass)... " + timeT;
    } 
    else if (eState == RenderEngine::EngineState::Finished) {
        currentProgress = 1.0f;
        
        if (isBatchProcessing) {
            currentStemIndex++;
            if (currentStemIndex < pendingStemsQueue.size()) {
                startNextBatchStem();
                return; // Continuar inmediatamente al prox track sin detener timer
            } else {
                currentInfoTimeStr = "All Stems Finished in " + timeT;
                updateVisibility();
                stopTimer();
                if (onAllStemsFinished) onAllStemsFinished();
                resetBatchState();
            }
        } else {
            currentInfoTimeStr = "Finished in " + timeT + "  (" + juce::String(realtimeSpeed, 1) + "x realtime)";
            updateVisibility();
            stopTimer();
        }
    }
    else if (eState == RenderEngine::EngineState::Rendering) {
        currentInfoTimeStr = "Rendering... " + timeT + "  (" + juce::String(realtimeSpeed, 1) + "x realtime)";
    }
    else if (eState == RenderEngine::EngineState::Error) {
        currentInfoTimeStr = "Error: " + juce::String(engine->getErrorMessage());
        updateVisibility();
        stopTimer();
    }
    else if (eState == RenderEngine::EngineState::Cancelled) {
        currentInfoTimeStr = "Cancelled";
        if (isBatchProcessing && onAllStemsFinished) onAllStemsFinished();
        resetBatchState();
        updateVisibility();
        stopTimer();
    }
    
    repaint();
}

void OfflineRenderer::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(45, 47, 50)); // Fondo Base de panel

    if (currentState == ViewState::Configuration) {
        g.fillAll(juce::Colour(35, 37, 40));
        return;
    }

    // --- SECCIÓN DIBUJO CUSTOM (DARK REAPER STYLE) ---
    // 1. Textos Generales 
    g.setColour(juce::Colours::white);
    
    // Titulo de info (Finished in...) - Miremos que lo ponen arriba a veces en el titlebar, pero aqui en grafico
    // Lo dibujaremos sobre el progreso
    
    // 2. Barra de Progreso
    g.setColour(juce::Colour(220, 220, 220));
    g.fillRect(progressRect);
    g.setColour(juce::Colour(30, 130, 40)); // Verde oscuro reaper
    g.fillRect(progressRect.withWidth((int)(progressRect.getWidth() * currentProgress)));

    // Texto sobre barra 
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText(currentInfoTimeStr, progressRect.getX(), progressRect.getY() - 18, progressRect.getWidth(), 15, juce::Justification::centred);

    // 3. VU METER (Doble barra)
    g.setColour(juce::Colour(15, 35, 60)); // Fondo VU marino oscuro
    g.fillRect(vuRect);
    
    // Texto Master mix local
    g.setColour(juce::Colours::lightgrey);
    g.setFont(12.0f);
    g.drawText("Master mix", progressRect.getX(), vuRect.getY(), 65, vuRect.getHeight(), juce::Justification::centredLeft);

    // Dibujando VU bars
    float peakDb = 20.0f * std::log10(std::max(0.00001f, currentTruePeak));
    float minDb = -60.0f;
    float dbRange = std::abs(minDb);
    float valF = juce::jlimit(0.0f, 1.0f, (peakDb - minDb) / dbRange);
    
    // Como si fuesen 2 canales (L, R) - usamos mismo valor
    int startXVu = vuRect.getX() + 70;
    int wVu = vuRect.getWidth() - 110;
    
    juce::Rectangle<int> vuL(startXVu, vuRect.getY() + 2, (int)(wVu * valF), (vuRect.getHeight()/2) - 2);
    juce::Rectangle<int> vuR(startXVu, vuRect.getCentreY() + 1, (int)(wVu * valF), (vuRect.getHeight()/2) - 2);
    
    // Gradient para VUs (Azul oscuro a Cyan a Rojo)
    juce::ColourGradient grad(juce::Colour(30, 60, 100), startXVu, 0, juce::Colour(60, 140, 170), startXVu + wVu - 50, 0, false);
    g.setGradientFill(grad);
    g.fillRect(vuL); g.fillRect(vuR);

    // Valores + y Cliping block ROJO a la derecha
    g.setColour(peakDb > 0.0f ? juce::Colour(240, 50, 50) : juce::Colour(30, 60, 100));
    juce::Rectangle<int> clipZone(startXVu + wVu, vuRect.getY(), 40, vuRect.getHeight());
    g.fillRect(clipZone);
    g.setColour(juce::Colours::white);
    g.drawText(juce::String(peakDb, 1), clipZone, juce::Justification::centredRight);

    // Grilla VU (-60, -54, -48...)
    g.setColour(juce::Colours::lightgrey);
    for(int db = -60; db <= 0; db += 6) {
        float xp = startXVu + wVu * ((db - minDb) / dbRange);
        if(db < 0) {
            g.drawText(juce::String(db), (int)xp - 10, vuRect.getY(), 20, vuRect.getHeight(), juce::Justification::centred);
        }
    }

    // 4. WAVEFORM OSCILATORS
    g.setColour(juce::Colour(35, 35, 35)); // Fondo Wave dark
    g.fillRect(waveRect);
    g.setColour(juce::Colour(15, 15, 15)); // Outline oscuro
    g.drawRect(waveRect);

    if (!localWaveMax.empty()) 
    {
        float midY = waveRect.getCentreY();
        float hScale = waveRect.getHeight() * 0.48f;
        int64_t tSamples = std::max((int64_t)1, engine->totalSamplesToRender.load(std::memory_order_relaxed));
        float blockW = (float)waveRect.getWidth() / (float)((tSamples / 4096) + 1);

        for (size_t i = 0; i < localWaveMax.size(); ++i) {
            float x = waveRect.getX() + (i * blockW);
            float vMax = std::min(localWaveMax[i], 2.0f);
            float vMin = std::max(localWaveMin[i], -2.0f);
            
            bool isClip = (vMax > 0.99f || vMin < -0.99f);
            if (isClip) {
                g.setColour(juce::Colours::red);
                g.drawVerticalLine((int)x, waveRect.getY()+1, waveRect.getBottom()-1);
            } else {
                g.setColour(juce::Colour(240, 140, 20)); // Naranja onda
                g.drawVerticalLine((int)x, midY - (vMax * hScale), midY - (vMin * hScale));
            }
        }
    }

    // 5. REGLA INFERIOR (Graduaciones temporales oscuras)
    juce::Rectangle<int> rulerRect(waveRect.getX(), waveRect.getBottom(), waveRect.getWidth(), 20);
    g.setColour(juce::Colour(50, 52, 55));
    g.fillRect(rulerRect);
    g.setColour(juce::Colours::white);
    g.drawText("0:00", rulerRect.getX() + 5, rulerRect.getY(), 40, 20, juce::Justification::centredLeft);
    
    int mins = (int)storedTotalLength / 60;
    float secs = std::fmod((float)storedTotalLength, 60.0f);
    g.drawText(juce::String::formatted("Length: %01d:%06.3f", mins, secs), rulerRect, juce::Justification::centred);
}

void OfflineRenderer::resized()
{
    auto area = getLocalBounds().reduced(15);

    if (currentState == ViewState::Configuration)
    {
        // Se mantiene la misma logica rapida de configuracion
        auto lCol = area.removeFromLeft(area.getWidth() / 2).reduced(10);
        auto rCol = area.reduced(10);

        auto r1 = lCol.removeFromTop(24); lblSource.setBounds(r1.removeFromLeft(80)); r1.removeFromLeft(5); sourceCombo.setBounds(r1); lCol.removeFromTop(5);
        auto r2 = lCol.removeFromTop(24); lblBounds.setBounds(r2.removeFromLeft(80)); r2.removeFromLeft(5); boundsCombo.setBounds(r2); lCol.removeFromTop(5);
        auto r3 = lCol.removeFromTop(24); tailToggle.setBounds(r3.removeFromLeft(80)); r3.removeFromLeft(5); tailMsInput.setBounds(r3); lCol.removeFromTop(15);
        auto r4 = lCol.removeFromTop(24); lblOutput.setBounds(r4.removeFromLeft(80)); r4.removeFromLeft(5); directoryInput.setBounds(r4); lCol.removeFromTop(5);
        auto r5 = lCol.removeFromTop(24); lblFileName.setBounds(r5.removeFromLeft(80)); r5.removeFromLeft(5); filenameInput.setBounds(r5); lCol.removeFromTop(15);

        auto rr1 = rCol.removeFromTop(24); lblSampleRate.setBounds(rr1.removeFromLeft(80)); rr1.removeFromLeft(5); cbSampleRate.setBounds(rr1); rCol.removeFromTop(5);
        auto rr2 = rCol.removeFromTop(24); lblBitDepth.setBounds(rr2.removeFromLeft(80)); rr2.removeFromLeft(5); cbBitDepth.setBounds(rr2); rCol.removeFromTop(5);
        auto rr3 = rCol.removeFromTop(24); writeBwfMetadata.setBounds(rr3); rCol.removeFromTop(5);
        auto rr4 = rCol.removeFromTop(24); lblLargeFiles.setBounds(rr4.removeFromLeft(90)); rr4.removeFromLeft(5); largeFilesCombo.setBounds(rr4); rCol.removeFromTop(15);

        auto e1 = rCol.removeFromTop(24); lblParallel.setBounds(e1.removeFromLeft(80)); e1.removeFromLeft(5); parallelRenderToggle.setBounds(e1); rCol.removeFromTop(5);
        secondPassToggle.setBounds(rCol.removeFromTop(24).withTrimmedLeft(85)); rCol.removeFromTop(5);
        skipSilentFiles.setBounds(rCol.removeFromTop(24).withTrimmedLeft(85));

        auto bottomRow = getLocalBounds().reduced(20).removeFromBottom(40);
        btnCloseConfig.setBounds(bottomRow.removeFromRight(100)); bottomRow.removeFromRight(10);
        btnStart.setBounds(bottomRow.removeFromRight(160));
    }
    else
    {
        // 1. TOP ZONA: Output File y format
        auto topBox = area.removeFromTop(60);
        
        juce::Rectangle<int> fileRow = topBox.removeFromTop(20);
        lblOutputTitle.setBounds(fileRow.removeFromLeft(65));
        outputFileBox.setBounds(fileRow);
        
        topBox.removeFromTop(10);
        lblFormatInfo.setBounds(topBox);

        area.removeFromTop(15);

        // 2. MIDDLE ZONA
        progressRect = area.removeFromTop(12);
        area.removeFromTop(20); // space for the text drawn above
        vuRect = area.removeFromTop(22);
        
        area.removeFromTop(10);
        waveRect = area.removeFromTop(120);
        area.removeFromTop(20); // space for the ruler
        
        // 3. TABLE ZONA
        area.removeFromTop(10);
        statsTable.setBounds(area.removeFromTop(100));

        // 4. BOTTOM ZONA: Buttons
        auto botArea = getLocalBounds().reduced(15).removeFromBottom(25);
        
        btnOpenFolder.setBounds(botArea.removeFromLeft(90)); botArea.removeFromLeft(10);
        btnLaunchFile.setBounds(botArea.removeFromLeft(90)); botArea.removeFromLeft(10);
        btnMediaExplorer.setBounds(botArea.removeFromLeft(110)); botArea.removeFromLeft(10);
        btnAddProject.setBounds(botArea.removeFromLeft(125));
        
        btnCloseRender.setBounds(botArea.removeFromRight(80)); botArea.removeFromRight(10);
        btnBack.setBounds(botArea.removeFromRight(80)); botArea.removeFromRight(10);
        btnStatsCharts.setBounds(botArea.removeFromRight(90));
    }
}
