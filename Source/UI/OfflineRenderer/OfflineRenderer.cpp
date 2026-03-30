#include "OfflineRenderer.h"

OfflineRenderer::OfflineRenderer() : juce::Thread("DawOfflineRenderThread")
{
    setOpaque(true);

    statusLabel.setText("Preparando renderizado...", juce::dontSendNotification);
    statusLabel.setFont(juce::Font("Inter", 16.0f, juce::Font::bold));
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    statsLabel.setText("LUFS-I: -- | True Peak: -- | Clips: 0", juce::dontSendNotification);
    statsLabel.setFont(juce::Font("Inter", 14.0f, juce::Font::plain));
    statsLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    statsLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(statsLabel);

    btnCancel.setButtonText("Cancelar");
    btnCancel.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 42, 45));
    btnCancel.onClick = [this] { cancelRender(); };
    addAndMakeVisible(btnCancel);
}

OfflineRenderer::~OfflineRenderer()
{
    cancelRender();
}

void OfflineRenderer::startRender(const juce::File& outputFile, double sampleRate, int bitDepth, double totalLengthSecs)
{
    targetFile = outputFile;
    targetSampleRate = sampleRate;
    targetBitDepth = bitDepth;
    totalLengthSeconds = totalLengthSecs;
    totalSamplesToRender = (juce::int64)(totalLengthSeconds * targetSampleRate);

    samplesRendered = 0;
    isFinished = false;
    isCancelled = false;
    totalClips = 0;
    currentTruePeak = -100.0f;
    currentLufsI = -100.0f;

    waveMaxBuffer.clear();
    waveMinBuffer.clear();
    clipPositions.clear();

    // Iniciar hilo de UI para repintar
    startTimerHz(30);

    // Iniciar hilo de procesamiento a máxima velocidad
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
        statsLabel.setText("El archivo exportado estará incompleto.", juce::dontSendNotification);
        btnCancel.setButtonText("Cerrar");
        repaint();
    }
}

void OfflineRenderer::run()
{
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(new juce::FileOutputStream(targetFile), 
                                  targetSampleRate, 
                                  2, // Stereo
                                  targetBitDepth, 
                                  {}, 0));

    if (writer == nullptr)
    {
        juce::MessageManager::callAsync([this] { statusLabel.setText("Error al crear el archivo.", juce::dontSendNotification); });
        return;
    }

    juce::MessageManager::callAsync([this] { statusLabel.setText("Exportando Mixdown (Offline)...", juce::dontSendNotification); });

    // Bloque estándar de 4096 muestras para balancear velocidad y resolución visual
    const int renderBlockSize = 4096; 
    juce::AudioBuffer<float> renderBuffer(2, renderBlockSize);

    // TODO: Aquí inicializarías tu clase SimpleLoudness del plugin:
    // lufsAnalyzer.prepare(targetSampleRate, renderBlockSize);

    while (!threadShouldExit() && samplesRendered < totalSamplesToRender)
    {
        int numSamples = (int)juce::jmin((juce::int64)renderBlockSize, totalSamplesToRender - samplesRendered);
        
        renderBuffer.setSize(2, numSamples, false, false, true);
        renderBuffer.clear();

        // 1. PEDIR AUDIO AL MOTOR DEL DAW
        if (onProcessOfflineBlock) {
            onProcessOfflineBlock(renderBuffer, numSamples);
        }

        // 2. EXTRACCIÓN DE DATOS PARA UI Y ANÁLISIS DE CLIPPING
        float minL = 0.0f, maxL = 0.0f, minR = 0.0f, maxR = 0.0f;
        renderBuffer.findMinMax(0, 0, numSamples, minL, maxL);
        renderBuffer.findMinMax(1, 0, numSamples, minR, maxR);
        
        float blockMax = std::max(std::abs(maxL), std::abs(maxR));
        float blockMin = std::max(std::abs(minL), std::abs(minR)); 

        bool clipped = (blockMax > 1.0f || blockMin > 1.0f); // > 0 dBFS

        // TODO: lufsAnalyzer.process(renderBuffer.getReadPointer(0), renderBuffer.getReadPointer(1), numSamples);

        {
            juce::ScopedLock sl(dataLock);
            
            waveMaxBuffer.push_back(blockMax);
            waveMinBuffer.push_back(-blockMin);

            if (blockMax > currentTruePeak) currentTruePeak = blockMax;

            if (clipped) {
                // Guardamos el segundo exacto donde ocurrió la distorsión
                double exactTimeSeconds = (double)samplesRendered / targetSampleRate;
                clipPositions.push_back(exactTimeSeconds);
                totalClips++;
            }
            
            // currentLufsI = lufsAnalyzer.getIntegratedLoudness();
        }

        // 3. ESCRIBIR A DISCO
        writer->writeFromAudioSampleBuffer(renderBuffer, 0, numSamples);
        samplesRendered += numSamples;
    }

    // Terminado
    if (!isCancelled) {
        isFinished = true;
        juce::MessageManager::callAsync([this] { 
            statusLabel.setText("¡Renderizado Completado!", juce::dontSendNotification); 
            statusLabel.setColour(juce::Label::textColourId, juce::Colours::limegreen);
            btnCancel.setButtonText("Cerrar");
        });
    }
}

void OfflineRenderer::timerCallback()
{
    // Actualizar UI a 30 FPS sin bloquear el hilo de exportación
    repaint();

    float progress = (float)((double)samplesRendered / (double)totalSamplesToRender);
    
    juce::ScopedLock sl(dataLock);
    
    juce::String clipText = (totalClips > 0) ? " | Clips: " + juce::String(totalClips) : " | Clips: 0";
    float peakDB = 20.0f * std::log10(currentTruePeak > 0.0001f ? currentTruePeak : 0.0001f);
    
    statsLabel.setText("Progreso: " + juce::String((int)(progress * 100.0f)) + "%" +
                       " | Pico: " + juce::String(peakDB, 1) + " dB" + clipText, juce::dontSendNotification);

    if (totalClips > 0) statsLabel.setColour(juce::Label::textColourId, juce::Colours::red);

    if (isFinished || isCancelled) {
        stopTimer();
    }
}

void OfflineRenderer::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(25, 27, 30));

    // Fondo del área del gráfico
    juce::Rectangle<int> graphArea(10, 50, getWidth() - 20, getHeight() - 110);
    g.setColour(juce::Colour(15, 17, 20));
    g.fillRect(graphArea);
    g.setColour(juce::Colours::grey.withAlpha(0.2f));
    g.drawRect(graphArea);

    juce::ScopedLock sl(dataLock);

    if (waveMaxBuffer.empty()) return;

    // 1. DIBUJAR LA ONDA PROGRESIVA
    float midY = graphArea.getCentreY();
    float heightScale = graphArea.getHeight() * 0.5f;
    float pixelsPerBlock = (float)graphArea.getWidth() / (float)((totalSamplesToRender / 4096) + 1);

    juce::Path wavePath;
    wavePath.startNewSubPath((float)graphArea.getX(), midY);

    for (size_t i = 0; i < waveMaxBuffer.size(); ++i) {
        float x = graphArea.getX() + (i * pixelsPerBlock);
        
        // Limitar visualmente el tope para que la onda no se salga de la caja
        float maxDraw = std::min(waveMaxBuffer[i], 1.0f);
        float yTop = midY - (maxDraw * heightScale);
        
        wavePath.lineTo(x, yTop);
    }
    
    for (int i = (int)waveMinBuffer.size() - 1; i >= 0; --i) {
        float x = graphArea.getX() + (i * pixelsPerBlock);
        float minDraw = std::max(waveMinBuffer[i], -1.0f);
        float yBot = midY - (minDraw * heightScale);
        
        wavePath.lineTo(x, yBot);
    }
    wavePath.closeSubPath();

    g.setColour(juce::Colours::cyan.withAlpha(0.6f));
    g.fillPath(wavePath);

    // 2. DIBUJAR MARCADORES ROJOS DE CLIPPING ESTILO REAPER
    g.setColour(juce::Colours::red);
    for (double clipTime : clipPositions) {
        float normalizedPos = (float)(clipTime / totalLengthSeconds);
        float x = graphArea.getX() + (normalizedPos * graphArea.getWidth());
        
        // Dibuja una línea vertical roja brillante que cruza toda la onda
        g.drawVerticalLine((int)x, (float)graphArea.getY(), (float)graphArea.getBottom());
    }
}

void OfflineRenderer::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    auto topArea = area.removeFromTop(30);
    statusLabel.setBounds(topArea.removeFromLeft(300));
    statsLabel.setBounds(topArea);

    auto bottomArea = area.removeFromBottom(40);
    btnCancel.setBounds(bottomArea.removeFromRight(100).reduced(0, 5));
}