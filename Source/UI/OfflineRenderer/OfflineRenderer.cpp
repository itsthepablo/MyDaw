#include "OfflineRenderer.h"

OfflineRenderer::OfflineRenderer() : juce::Thread("DawOfflineRenderThread")
{
    setOpaque(true);

    // --- SETUP VISTA CONFIGURACIÓN ---
    lblTitle.setFont(juce::Font("Inter", 24.0f, juce::Font::bold));
    lblTitle.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(lblTitle);

    lblSampleRate.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(lblSampleRate);

    cbSampleRate.addItem("44100 Hz", 1);
    cbSampleRate.addItem("48000 Hz", 2);
    cbSampleRate.addItem("88200 Hz", 3);
    cbSampleRate.addItem("96000 Hz", 4);
    cbSampleRate.setSelectedId(1);
    addAndMakeVisible(cbSampleRate);

    lblBitDepth.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(lblBitDepth);

    cbBitDepth.addItem("16 bit PCM", 1);
    cbBitDepth.addItem("24 bit PCM", 2);
    cbBitDepth.addItem("32 bit FP", 3);
    cbBitDepth.setSelectedId(2);
    addAndMakeVisible(cbBitDepth);

    cbNormalize.setToggleState(false, juce::dontSendNotification);
    cbNormalize.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(cbNormalize);

    btnStart.setColour(juce::TextButton::buttonColourId, juce::Colour(70, 75, 80));
    btnStart.onClick = [this] {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Save Output File",
            juce::File::getSpecialLocation(juce::File::userMusicDirectory).getChildFile("Mixdown.wav"),
            "*.wav"
        );

        fileChooser->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& fc) {
                auto file = fc.getResult();
                if (file != juce::File{}) {
                    double sr = cbSampleRate.getText().substring(0, 5).getDoubleValue();
                    int bd = cbBitDepth.getText().substring(0, 2).getIntValue();
                    startRender(file, sr, bd, cbNormalize.getToggleState());
                }
            });
        };
    addAndMakeVisible(btnStart);

    btnCloseConfig.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 42, 45));
    btnCloseConfig.onClick = [this] { if (onClose) onClose(); };
    addAndMakeVisible(btnCloseConfig);


    // --- SETUP VISTA RENDERIZADO (REAPER STYLE) ---
    statusLabel.setFont(juce::Font("Inter", 17.0f, juce::Font::bold));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addChildComponent(statusLabel);

    timeLabel.setFont(juce::Font("Inter", 14.0f, juce::Font::plain));
    timeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    timeLabel.setJustificationType(juce::Justification::centredRight);
    addChildComponent(timeLabel);

    statsLabel.setFont(juce::Font("Consolas", 13.0f, juce::Font::plain));
    statsLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    statsLabel.setJustificationType(juce::Justification::centred);
    addChildComponent(statsLabel);

    btnCancelRender.setButtonText("Cancelar");
    btnCancelRender.setColour(juce::TextButton::buttonColourId, juce::Colour(50, 55, 60));
    btnCancelRender.onClick = [this] {
        if (isFinished || isCancelled) { if (onClose) onClose(); }
        else { cancelRender(); }
    };
    addChildComponent(btnCancelRender);

    btnOpenFolder.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 65, 70));
    btnOpenFolder.onClick = [this] { targetFile.getParentDirectory().startAsProcess(); };
    addChildComponent(btnOpenFolder);

    btnLaunchFile.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 65, 70));
    btnLaunchFile.onClick = [this] { targetFile.startAsProcess(); };
    addChildComponent(btnLaunchFile);

    updateVisibility();
}

OfflineRenderer::~OfflineRenderer() { cancelRender(); }

void OfflineRenderer::updateVisibility()
{
    bool isConfig = (currentState == ViewState::Configuration);

    lblTitle.setVisible(isConfig);
    lblSampleRate.setVisible(isConfig);
    cbSampleRate.setVisible(isConfig);
    lblBitDepth.setVisible(isConfig);
    cbBitDepth.setVisible(isConfig);
    cbNormalize.setVisible(isConfig);
    btnStart.setVisible(isConfig);
    btnCloseConfig.setVisible(isConfig);

    statusLabel.setVisible(!isConfig);
    timeLabel.setVisible(!isConfig);
    statsLabel.setVisible(!isConfig);
    btnCancelRender.setVisible(!isConfig);
    
    // Solo visibles al terminar
    bool showPostRender = (!isConfig && (isFinished || isCancelled));
    btnOpenFolder.setVisible(showPostRender);
    btnLaunchFile.setVisible(showPostRender);

    resized();
    repaint();
}

void OfflineRenderer::showConfig(double totalLengthSecs)
{
    totalLengthSeconds = totalLengthSecs;
    currentState = ViewState::Configuration;
    updateVisibility();
}

void OfflineRenderer::startRender(const juce::File& outputFile, double sampleRate, int bitDepth, bool normalize)
{
    targetFile = outputFile;
    targetSampleRate = sampleRate;
    targetBitDepth = bitDepth;
    shouldNormalize = normalize;
    totalSamplesToRender = (juce::int64)(totalLengthSeconds * targetSampleRate);

    samplesRendered = 0;
    isFinished = false;
    isCancelled = false;
    isNormalizing = false;
    totalClips = 0;
    currentTruePeak = 0.0f;
    
    loudnessMeter.prepare(targetSampleRate, 4096);

    waveMaxBuffer.clear();
    waveMinBuffer.clear();
    clipPositions.clear();

    btnCancelRender.setButtonText("Cancelar");
    statusLabel.setText("Renderizado en progreso...", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    currentState = ViewState::Rendering;
    updateVisibility();

    // Notificar al motor para que se prepare (Rule #18)
    if (onPrepareEngine) onPrepareEngine(targetSampleRate);

    startTimerHz(30);
    startThread(juce::Thread::Priority::highest);
}

void OfflineRenderer::cancelRender()
{
    if (isThreadRunning())
    {
        isCancelled = true;
        signalThreadShouldExit();
        stopThread(2000);

        statusLabel.setText("Renderizado Cancelado.", juce::dontSendNotification);
        btnCancelRender.setButtonText("Cerrar");
        updateVisibility();
    }
}

void OfflineRenderer::run()
{
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(new juce::FileOutputStream(targetFile),
            targetSampleRate, 2, targetBitDepth, {}, 0));

    if (writer == nullptr) {
        juce::MessageManager::callAsync([this] { statusLabel.setText("Error: No se pudo crear el archivo.", juce::dontSendNotification); });
        return;
    }

    const int renderBlockSize = 4096;
    juce::AudioBuffer<float> renderBuffer(2, renderBlockSize);

    auto startTime = juce::Time::getMillisecondCounterHiRes();

    while (!threadShouldExit() && samplesRendered < totalSamplesToRender)
    {
        int numSamples = (int)juce::jmin((juce::int64)renderBlockSize, totalSamplesToRender - samplesRendered);
        renderBuffer.setSize(2, numSamples, false, false, true);
        renderBuffer.clear();

        if (onProcessOfflineBlock) onProcessOfflineBlock(renderBuffer, numSamples);

        // --- ANALISIS REAPER STYLE ---
        loudnessMeter.process(renderBuffer);
        
        auto rangeL = renderBuffer.findMinMax(0, 0, numSamples);
        auto rangeR = renderBuffer.findMinMax(1, 0, numSamples);
        float blockMax = std::max(rangeL.getEnd(), rangeR.getEnd());
        float blockMin = std::min(rangeL.getStart(), rangeR.getStart());
        float absMax = std::max(std::abs(blockMax), std::abs(blockMin));

        {
            juce::ScopedLock sl(dataLock);
            waveMaxBuffer.push_back(blockMax);
            waveMinBuffer.push_back(blockMin);
            if (absMax > currentTruePeak) currentTruePeak = absMax;
            
            if (absMax > 1.0f) {
                double exactTimeSeconds = (double)samplesRendered / targetSampleRate;
                clipPositions.push_back(exactTimeSeconds);
                totalClips++;
            }
            
            currentLufsM = loudnessMeter.getShortTerm(); // Usamos short-term como aproximación rápida a momentary
            currentLufsS = loudnessMeter.getShortTerm();
            currentLufsI = loudnessMeter.getIntegrated();
            currentLra = loudnessMeter.getRange();
        }

        writer->writeFromAudioSampleBuffer(renderBuffer, 0, numSamples);
        samplesRendered += numSamples;
    }

    writer.reset(); // Cerramos el archivo para lectura/escritura posterior

    if (!isCancelled) {
        if (shouldNormalize && currentTruePeak > 0.0001f && currentTruePeak != 1.0f) {
            isNormalizing = true;
            juce::MessageManager::callAsync([this] { statusLabel.setText("Normalizando archivo...", juce::dontSendNotification); });
            performNormalization();
        }
        
        isFinished = true;
        auto endTime = juce::Time::getMillisecondCounterHiRes();
        double elapsed = (endTime - startTime) / 1000.0;
        double realtimeSpeed = totalLengthSeconds / elapsed;

        juce::MessageManager::callAsync([this, elapsed, realtimeSpeed] {
            statusLabel.setText("¡Renderizado Completado!", juce::dontSendNotification);
            statusLabel.setColour(juce::Label::textColourId, juce::Colours::limegreen);
            
            juce::String timeStr = "Finished in " + juce::String(elapsed, 1) + "s (" + juce::String(realtimeSpeed, 1) + "x realtime)";
            timeLabel.setText(timeStr, juce::dontSendNotification);
            
            btnCancelRender.setButtonText("Cerrar");
            updateVisibility();
        });
    }
}

void OfflineRenderer::performNormalization()
{
    float gain = 1.0f / currentTruePeak;
    
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatReader> reader(wavFormat.createReaderFor(new juce::FileInputStream(targetFile), true));
    
    if (reader) {
        juce::File tempFile = targetFile.getSiblingFile(targetFile.getFileNameWithoutExtension() + "_norm.wav");
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(new juce::FileOutputStream(tempFile),
                targetSampleRate, 2, targetBitDepth, {}, 0));
                
        if (writer) {
            int64_t totalS = reader->lengthInSamples;
            const int blockSize = 16384;
            juce::AudioBuffer<float> buffer(2, blockSize);
            
            for (int64_t s = 0; s < totalS; s += blockSize) {
                int num = (int)juce::jmin((int64_t)blockSize, totalS - s);
                reader->read(&buffer, 0, num, s, true, true);
                buffer.applyGain(0, num, gain);
                writer->writeFromAudioSampleBuffer(buffer, 0, num);
            }
            writer.reset();
            reader.reset();
            
            targetFile.deleteFile();
            tempFile.moveFileTo(targetFile);
            
            currentTruePeak = 1.0f; // Ahora es 0dB
        }
    }
    isNormalizing = false;
}

void OfflineRenderer::timerCallback()
{
    if (currentState != ViewState::Rendering) return;
    
    juce::ScopedLock sl(dataLock);
    
    float peakDb = 20.0f * std::log10(std::max(0.00001f, currentTruePeak));
    juce::String peakStr = (peakDb > 0.0f) ? "+" + juce::String(peakDb, 1) : juce::String(peakDb, 1);
    
    juce::String stats = "PEAK: " + peakStr + " dB | CLIPS: " + juce::String(totalClips) + 
                         " | LUFS-M: " + juce::String(currentLufsM, 1) + 
                         " | LUFS-S: " + juce::String(currentLufsS, 1) + 
                         " | LUFS-I: " + juce::String(currentLufsI, 1) + 
                         " | LRA: " + juce::String(currentLra, 1);
    
    statsLabel.setText(stats, juce::dontSendNotification);
    if (totalClips > 0) statsLabel.setColour(juce::Label::textColourId, juce::Colours::orangered);
    
    if (isFinished || isCancelled) stopTimer();
    repaint();
}

void OfflineRenderer::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(35, 37, 40));

    if (currentState == ViewState::Configuration) {
        juce::Rectangle<int> box(20, 70, getWidth() - 40, getHeight() - 150);
        g.setColour(juce::Colour(25, 27, 30));
        g.fillRoundedRectangle(box.toFloat(), 6.0f);
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(box.toFloat(), 6.0f, 1.0f);
        return;
    }

    // --- GRÁFICO REAPER STYLE ---
    juce::Rectangle<int> graphArea(15, 60, getWidth() - 30, getHeight() - 160);
    g.setColour(juce::Colours::black);
    g.fillRect(graphArea);
    
    juce::ScopedLock sl(dataLock);
    if (waveMaxBuffer.empty()) return;

    float midY = graphArea.getCentreY();
    float hScale = graphArea.getHeight() * 0.45f;
    float blockW = (float)graphArea.getWidth() / (float)((totalSamplesToRender / 4096) + 1);

    for (size_t i = 0; i < waveMaxBuffer.size(); ++i) {
        float x = graphArea.getX() + (i * blockW);
        float vMax = std::min(waveMaxBuffer[i], 2.0f);
        float vMin = std::max(waveMinBuffer[i], -2.0f);
        
        bool isClip = (std::abs(vMax) > 1.0f || std::abs(vMin) > 1.0f);
        g.setColour(isClip ? juce::Colours::red : juce::Colours::orange.withAlpha(0.8f));
        g.drawVerticalLine((int)x, midY - (vMax * hScale), midY - (vMin * hScale));
    }

    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawRect(graphArea, 1);
}

void OfflineRenderer::resized()
{
    auto area = getLocalBounds().reduced(20);

    if (currentState == ViewState::Configuration)
    {
        lblTitle.setBounds(area.removeFromTop(40));
        area.removeFromTop(20);

        auto row1 = area.removeFromTop(30);
        lblSampleRate.setBounds(row1.removeFromLeft(120));
        row1.removeFromLeft(10);
        cbSampleRate.setBounds(row1.removeFromLeft(150));

        area.removeFromTop(10);

        auto row2 = area.removeFromTop(30);
        lblBitDepth.setBounds(row2.removeFromLeft(120));
        row2.removeFromLeft(10);
        cbBitDepth.setBounds(row2.removeFromLeft(150));
        
        area.removeFromTop(15);
        cbNormalize.setBounds(area.removeFromTop(30).withSizeKeepingCentre(200, 30));

        auto bottomRow = getLocalBounds().reduced(20).removeFromBottom(40);
        btnCloseConfig.setBounds(bottomRow.removeFromRight(100));
        bottomRow.removeFromRight(10);
        btnStart.setBounds(bottomRow.removeFromRight(160));
    }
    else
    {
        auto topArea = area.removeFromTop(35);
        statusLabel.setBounds(topArea.removeFromLeft(300));
        timeLabel.setBounds(topArea);

        auto bottomArea = getLocalBounds().reduced(15).removeFromBottom(40);
        btnCancelRender.setBounds(bottomArea.removeFromRight(110));
        
        if (isFinished || isCancelled) {
            bottomArea.removeFromRight(10);
            btnLaunchFile.setBounds(bottomArea.removeFromRight(130));
            bottomArea.removeFromRight(10);
            btnOpenFolder.setBounds(bottomArea.removeFromRight(130));
        }

        statsLabel.setBounds(getLocalBounds().removeFromBottom(85).removeFromTop(25));
    }
}