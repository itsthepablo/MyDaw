#include "RenderEngine.h"

RenderEngine::RenderEngine() : juce::Thread("DawOfflineRenderThread")
{
}

RenderEngine::~RenderEngine()
{
    cancelRender();
}

void RenderEngine::startRender(const juce::File& outputFile, double sampleRate, int bitDepth, bool normalize, double totalLengthSecs)
{
    targetFile = outputFile;
    targetSampleRate = sampleRate;
    targetBitDepth = bitDepth;
    shouldNormalize = normalize;
    totalLengthSeconds = totalLengthSecs;
    
    totalSamplesToRender = (int64_t)(totalLengthSeconds * targetSampleRate);
    samplesRendered = 0;
    totalClips = 0;
    
    // Preparar modulos
    loudnessMeter.prepare(targetSampleRate, 4096);
    
    currentLufsM = -100.0f;
    currentLufsS = -100.0f;
    currentLufsI = -100.0f;
    currentLra = 0.0f;
    currentTruePeak = 0.0f;
    errorMessage = "";
    
    {
        juce::ScopedLock sl(waveDataLock);
        waveMaxBuffer.clear();
        waveMinBuffer.clear();
    }
    
    if (onPrepareEngine) onPrepareEngine(targetSampleRate);
    
    currentState = EngineState::Rendering;
    startThread(juce::Thread::Priority::highest);
}

void RenderEngine::cancelRender()
{
    if (isThreadRunning())
    {
        signalThreadShouldExit();
        stopThread(2000);
        currentState = EngineState::Cancelled;
    }
}

void RenderEngine::getWaveformSnapshot(std::vector<float>& maxBuf, std::vector<float>& minBuf)
{
    juce::ScopedLock sl(waveDataLock);
    maxBuf = waveMaxBuffer;
    minBuf = waveMinBuffer;
}

void RenderEngine::run()
{
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wavFormat.createWriterFor(new juce::FileOutputStream(targetFile),
                                  targetSampleRate, 2, targetBitDepth, {}, 0));

    if (writer == nullptr) {
        errorMessage = "Error: No se pudo crear el archivo WAV destino.";
        currentState = EngineState::Error;
        if (onRenderFinished) juce::MessageManager::callAsync([this] { onRenderFinished(false); });
        return;
    }

    const int renderBlockSize = 4096;
    juce::AudioBuffer<float> renderBuffer(2, renderBlockSize);
    juce::AudioBuffer<float> analysisBuffer(2, renderBlockSize); // <-- COPIA PURA PARA LUFS
    
    while (!threadShouldExit() && samplesRendered.load() < totalSamplesToRender.load())
    {
        int numSamples = (int)juce::jmin((int64_t)renderBlockSize, totalSamplesToRender.load() - samplesRendered.load());
        renderBuffer.setSize(2, numSamples, false, false, true);
        renderBuffer.clear();

        // 1. Proceso Principal DSP Crudo (Bit a Bit)
        if (onProcessOfflineBlock) onProcessOfflineBlock(renderBuffer, numSamples);

        // Copiamos la señal inmaculada porque los medidores EBU y True Peak API aplican EQs destructivamente para el vúmetro
        analysisBuffer.setSize(2, numSamples, false, false, true);
        for (int ch = 0; ch < renderBuffer.getNumChannels(); ++ch) {
            analysisBuffer.copyFrom(ch, 0, renderBuffer, ch, 0, numSamples);
        }

        // 2. Módulos de Análisis (Loudness EBU R128 K-Weighted filter destruye la señal)
        loudnessMeter.process(analysisBuffer);
        
        currentLufsM.store(loudnessMeter.getShortTerm(), std::memory_order_relaxed); 
        currentLufsS.store(loudnessMeter.getShortTerm(), std::memory_order_relaxed);
        currentLufsI.store(loudnessMeter.getIntegrated(), std::memory_order_relaxed);
        currentLra.store(loudnessMeter.getRange(), std::memory_order_relaxed);
        
        // 3. Extracción visual y picos digitales crudos (Detección de Clipping clásico)
        auto rangeL = renderBuffer.findMinMax(0, 0, numSamples);
        auto rangeR = renderBuffer.findMinMax(1, 0, numSamples);
        float blockMax = std::max(rangeL.getEnd(), rangeR.getEnd());
        float blockMin = std::min(rangeL.getStart(), rangeR.getStart());
        float absMax = std::max(std::abs(blockMax), std::abs(blockMin));
        
        if (absMax > currentTruePeak.load(std::memory_order_relaxed)) {
            currentTruePeak.store(absMax, std::memory_order_relaxed);
        }
        
        if (absMax > 1.0f) {
            totalClips++;
        }

        {
            juce::ScopedLock sl(waveDataLock);
            waveMaxBuffer.push_back(blockMax);
            waveMinBuffer.push_back(blockMin);
        }

        writer->writeFromAudioSampleBuffer(renderBuffer, 0, numSamples);
        samplesRendered += numSamples;
    }

    writer.reset(); // Cerramos archivo para evitar access violations en 2nd pass

    if (!threadShouldExit())
    {
        if (shouldNormalize && currentTruePeak.load() > 0.0001f && currentTruePeak.load() != 1.0f) {
            currentState = EngineState::Normalizing;
            performNormalization();
        }
        currentState = EngineState::Finished;
        
        if (onRenderFinished) {
            bool success = (currentState.load() != EngineState::Error);
            juce::MessageManager::callAsync([this, success] { onRenderFinished(success); });
        }
    }
    else
    {
        if (onRenderFinished) {
            juce::MessageManager::callAsync([this] { onRenderFinished(false); });
        }
    }
}

void RenderEngine::performNormalization()
{
    float gain = 1.0f / currentTruePeak.load(std::memory_order_relaxed);
    
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
            
            currentTruePeak.store(1.0f, std::memory_order_relaxed);
        }
    }
}
