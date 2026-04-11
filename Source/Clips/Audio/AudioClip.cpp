#include "AudioClip.h"
#include <cmath>
#include <thread>

bool AudioClip::showWaveformDebugInfo = false;

AudioClip::AudioClip() 
{
    // Reservar 4 niveles de LOD (Mip-Maps)
    peaksL.resize(4);
    peaksR.resize(4);
    peaksMid.resize(4);
    peaksSide.resize(4);
}

AudioClip::AudioClip(const AudioClip& other)
{
    *this = other;
}

AudioClip& AudioClip::operator=(const AudioClip& other)
{
    if (this != &other) {
        name = other.name;
        startX = other.startX;
        width = other.width;
        offsetX = other.offsetX;
        originalWidth = other.originalWidth;
        isMuted = other.isMuted;
        isSelected = other.isSelected;
        style = other.style;
        isLoadedFlag.store(other.isLoadedFlag.load());
        sourceFilePath = other.sourceFilePath;
        fileBuffer.makeCopyOf(other.fileBuffer);
        sourceSampleRate = other.sourceSampleRate;
        peaksL = other.peaksL;
        peaksR = other.peaksR;
        peaksMid = other.peaksMid;
        peaksSide = other.peaksSide;
        spectrogramImage = other.spectrogramImage;
        // El thumbnail no se copia habitualmente
    }
    return *this;
}

bool AudioClip::loadFromFile(const juce::File& file, double targetSampleRate)
{
    juce::AudioFormatManager manager;
    manager.registerBasicFormats();
    
    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(file));
    
    if (reader != nullptr)
    {
        name = file.getFileNameWithoutExtension();
        sourceFilePath = file.getFullPathName();
        sourceSampleRate = reader->sampleRate;
        
        fileBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&fileBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
        
        // Calcular ancho base (160 pixels por segundo es nuestro estándar)
        double duration = (double)reader->lengthInSamples / reader->sampleRate;
        width = (float)(duration * 160.0);
        originalWidth = width;
        
        generateCache();
        isLoadedFlag.store(true);
        return true;
    }
    
    return false;
}

void AudioClip::generateCache()
{
    const int baseSamplesPerPoint = 256;
    const int numSamples = fileBuffer.getNumSamples();
    const int numChannels = fileBuffer.getNumChannels();
    if (numSamples <= 0 || numChannels <= 0) return;

    const int numBasePoints = (int)((numSamples + baseSamplesPerPoint - 1) / baseSamplesPerPoint);
    bool isStereo = numChannels > 1;

    // Resetear niveles
    for (int i = 0; i < 4; ++i) {
        peaksL[i].clear();
        peaksR[i].clear();
        peaksMid[i].clear();
        peaksSide[i].clear();
    }

    // --- LOD 0 (Base) ---
    peaksL[0].assign(numBasePoints, { 0.0f, 0.0f });
    if (isStereo) {
        peaksR[0].assign(numBasePoints, { 0.0f, 0.0f });
        peaksMid[0].assign(numBasePoints, { 0.0f, 0.0f });
        peaksSide[0].assign(numBasePoints, { 0.0f, 0.0f });
    }

    const float* lData = fileBuffer.getReadPointer(0);
    const float* rData = isStereo ? fileBuffer.getReadPointer(1) : nullptr;

    for (int i = 0; i < numBasePoints; ++i) {
        int startSample = i * baseSamplesPerPoint;
        int endSample = std::min(startSample + baseSamplesPerPoint, numSamples);

        AudioPeak pL, pR, pMid, pSide;
        for (int s = startSample; s < endSample; ++s) {
            float l = lData[s];
            pL.maxPos = std::max(pL.maxPos, l);
            pL.minNeg = std::min(pL.minNeg, l);

            if (isStereo) {
                float r = rData[s];
                pR.maxPos = std::max(pR.maxPos, r);
                pR.minNeg = std::min(pR.minNeg, r);

                float mid = (l + r) * 0.5f;
                pMid.maxPos = std::max(pMid.maxPos, mid);
                pMid.minNeg = std::min(pMid.minNeg, mid);

                float side = (l - r) * 0.5f;
                pSide.maxPos = std::max(pSide.maxPos, side);
                pSide.minNeg = std::min(pSide.minNeg, side);
            }
        }
        peaksL[0][i] = pL;
        if (isStereo) {
            peaksR[0][i] = pR;
            peaksMid[0][i] = pMid;
            peaksSide[0][i] = pSide;
        }
    }

    // --- LOD 1, 2, 3 ---
    for (int i = 1; i < 4; ++i) {
        peaksL[i] = buildLOD(peaksL[i - 1], 4);
        if (isStereo) {
            peaksR[i] = buildLOD(peaksR[i - 1], 4);
            peaksMid[i] = buildLOD(peaksMid[i - 1], 4);
            peaksSide[i] = buildLOD(peaksSide[i - 1], 4);
        }
    }

    // EL ESPECTROGRAMA YA NO SE GENERA AQUÍ. 
    // Ahora se genera bajo demanda mediante ensureSpectrogramIsGenerated().
    spectrogramGenerated.store(false);
    spectrogramInProgress.store(false);
}

std::vector<AudioPeak> AudioClip::buildLOD(const std::vector<AudioPeak>& source, int factor)
{
    if (source.empty()) return {};
    int numPoints = (int)(source.size() + factor - 1) / factor;
    std::vector<AudioPeak> lod(numPoints, { 0.0f, 0.0f });

    for (int i = 0; i < numPoints; ++i) {
        int start = i * factor;
        int end = std::min(start + factor, (int)source.size());
        AudioPeak combined = source[start];
        for (int j = start + 1; j < end; ++j) {
            combined.maxPos = std::max(combined.maxPos, source[j].maxPos);
            combined.minNeg = std::min(combined.minNeg, source[j].minNeg);
        }
        lod[i] = combined;
    }
    return lod;
}

void AudioClip::ensureSpectrogramIsGenerated()
{
    // Si ya está listo o ya hay uno en progreso, no hacemos nada
    if (spectrogramGenerated.load() || spectrogramInProgress.load())
        return;

    // Lanzar generación en segundo plano para no bloquear la UI
    spectrogramInProgress.store(true);
    
    // Usamos un hilo suelto (detached) para este procesamiento asíncrono.
    // En una app más compleja usaríamos un ThreadPool.
    std::thread([this]() {
        generateSpectrogram();
    }).detach();
}

void AudioClip::generateSpectrogram()
{
    const int numSamples = fileBuffer.getNumSamples();
    if (numSamples <= 0) return;

    auto getHeatColor = [](float intensity) -> juce::Colour {
        intensity = juce::jlimit(0.0f, 1.0f, intensity);
        // Paleta de Calor Profesional (Fuego/Magma)
        if (intensity < 0.1f) return juce::Colour(10, 12, 15).interpolatedWith(juce::Colours::darkred.darker(0.8f), intensity * 10.0f);
        if (intensity < 0.35f) return juce::Colours::darkred.darker(0.8f).interpolatedWith(juce::Colours::darkred, (intensity - 0.1f) / 0.25f);
        if (intensity < 0.6f) return juce::Colours::darkred.interpolatedWith(juce::Colours::orangered, (intensity - 0.35f) / 0.25f);
        if (intensity < 0.85f) return juce::Colours::orangered.interpolatedWith(juce::Colours::orange, (intensity - 0.6f) / 0.25f);
        return juce::Colours::orange.interpolatedWith(juce::Colours::yellow, (intensity - 0.85f) / 0.15f);
    };

    const int fftOrder = 11; // 2048 puntos para alta definición vertical
    const int fftSize = 1 << fftOrder;
    
    // --- RESOLUCION ADAPTATIVA ---
    // Si el archivo es muy largo, aumentamos el hopSize para no generar imagenes gigantescas
    // que tardan una eternidad en calcularse. Capamos a aprox 4096 columnas.
    int hopSize = std::max(64, numSamples / 4096); 
    spectrogramHopSize = (double)hopSize;

    juce::dsp::FFT fft(fftOrder);
    juce::dsp::WindowingFunction<float> window((size_t)fftSize, juce::dsp::WindowingFunction<float>::hann);

    int totalFrames = numSamples / hopSize;
    if (totalFrames > 0) {
        int imgHeight = fftSize / 2;
        
        // Creamos la imagen en el hilo de fondo
        juce::Image newImage(juce::Image::ARGB, totalFrames, imgHeight, true);
        juce::Image::BitmapData bmpData(newImage, juce::Image::BitmapData::writeOnly);

        std::vector<float> fftData((size_t)(fftSize * 2), 0.0f);
        const float* audioData = fileBuffer.getReadPointer(0);

        for (int frame = 0; frame < totalFrames; ++frame) {
            int startSample = frame * hopSize;
            std::fill(fftData.begin(), fftData.end(), 0.0f);
            for (int i = 0; i < fftSize; ++i) {
                if (startSample + i < numSamples) fftData[i] = audioData[startSample + i];
            }

            window.multiplyWithWindowingTable(fftData.data(), (size_t)fftSize);
            fft.performFrequencyOnlyForwardTransform(fftData.data());

            for (int bin = 0; bin < imgHeight; ++bin) {
                float rawMag = fftData[bin];
                float intensity = 0.0f;
                if (rawMag > 1e-7f) {
                    float dB = 20.0f * std::log10(std::max(1e-8f, rawMag));
                    intensity = juce::jlimit(0.0f, 1.0f, (dB + 80.0f) / 80.0f);
                }
                juce::Colour heat = getHeatColor(intensity);
                bmpData.setPixelColour(frame, imgHeight - bin - 1, heat);
            }
        }
        
        // Asignamos la imagen y marcamos como completa
        spectrogramImage = newImage;
    }
    
    spectrogramGenerated.store(true);
    spectrogramInProgress.store(false);
}

const std::vector<AudioPeak>& AudioClip::getPeaksL(int lod) const 
{ 
    static std::vector<AudioPeak> empty;
    return (lod >= 0 && lod < 4) ? peaksL[lod] : empty; 
}
const std::vector<AudioPeak>& AudioClip::getPeaksR(int lod) const 
{ 
    static std::vector<AudioPeak> empty;
    return (lod >= 0 && lod < 4) ? peaksR[lod] : empty; 
}
const std::vector<AudioPeak>& AudioClip::getPeaksMid(int lod) const 
{ 
    static std::vector<AudioPeak> empty;
    return (lod >= 0 && lod < 4) ? peaksMid[lod] : empty; 
}
const std::vector<AudioPeak>& AudioClip::getPeaksSide(int lod) const 
{ 
    static std::vector<AudioPeak> empty;
    return (lod >= 0 && lod < 4) ? peaksSide[lod] : empty; 
}
