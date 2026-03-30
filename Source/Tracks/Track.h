#pragma once
#include <JuceHeader.h>
#include "../GlobalData.h" 
#include "../Native_Plugins/BaseEffect.h"
#include "../PluginHost/VSTHost.h" 
#include "../Engine/SimpleLoudness.h"
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <atomic>

enum class TrackType { MIDI, Audio };
enum class WaveformViewMode { Combined, SeparateLR, MidSide, Spectrogram }; // Agregado Espectrograma

struct AudioPeak {
    float maxPos = 0.0f;
    float minNeg = 0.0f;
};

struct AudioClipData {
    juce::String name;
    float startX = 0.0f;
    float width = 0.0f;
    float offsetX = 0.0f;
    float originalWidth = 0.0f;
    bool isMuted = false;
    bool isSelected = false;
    std::atomic<bool> isLoaded { true }; // Empieza en true, el drag async lo pone false
    juce::String sourceFilePath; 
    juce::AudioBuffer<float> fileBuffer;
    double sourceSampleRate = 44100.0;

    std::vector<AudioPeak> cachedPeaksL;
    std::vector<AudioPeak> cachedPeaksR;
    std::vector<AudioPeak> cachedPeaksMid;
    std::vector<AudioPeak> cachedPeaksSide;
    
    juce::Image spectrogramImage; // Cache Matrix FFT

    void generateCache() {
        const int samplesPerPoint = 256;
        const int numSamples = fileBuffer.getNumSamples();
        if (numSamples <= 0) return;

        const int numPoints = numSamples / samplesPerPoint;

        cachedPeaksL.clear(); cachedPeaksL.reserve(numPoints);
        cachedPeaksR.clear(); cachedPeaksMid.clear(); cachedPeaksSide.clear();

        bool isStereo = fileBuffer.getNumChannels() > 1;
        if (isStereo) {
            cachedPeaksR.reserve(numPoints);
            cachedPeaksMid.reserve(numPoints);
            cachedPeaksSide.reserve(numPoints);
        }

        for (int i = 0; i < numPoints; ++i) {
            int startSample = i * samplesPerPoint;
            int numSamplesInPoint = std::min(samplesPerPoint, numSamples - startSample);

            AudioPeak pL, pR, pMid, pSide;

            if (isStereo) {
                const float* lData = fileBuffer.getReadPointer(0, startSample);
                const float* rData = fileBuffer.getReadPointer(1, startSample);

                for (int s = 0; s < numSamplesInPoint; ++s) {
                    float l = lData[s];
                    float r = rData[s];
                    pL.maxPos = std::max(pL.maxPos, l); pL.minNeg = std::min(pL.minNeg, l);
                    pR.maxPos = std::max(pR.maxPos, r); pR.minNeg = std::min(pR.minNeg, r);
                    
                    float mid = (l + r) * 0.5f;
                    pMid.maxPos = std::max(pMid.maxPos, mid); pMid.minNeg = std::min(pMid.minNeg, mid);
                    
                    float side = (l - r) * 0.5f;
                    pSide.maxPos = std::max(pSide.maxPos, side); pSide.minNeg = std::min(pSide.minNeg, side);
                }
                cachedPeaksL.push_back(pL);
                cachedPeaksR.push_back(pR);
                cachedPeaksMid.push_back(pMid);
                cachedPeaksSide.push_back(pSide);
            }
            else {
                const float* lData = fileBuffer.getReadPointer(0, startSample);
                for (int s = 0; s < numSamplesInPoint; ++s) {
                    float l = lData[s];
                    pL.maxPos = std::max(pL.maxPos, l); pL.minNeg = std::min(pL.minNeg, l);
                }
                cachedPeaksL.push_back(pL);
            }
        }
        
        // --- FFT SPECTROGRAM GENERATION ---
        auto getHeatColor = [](float magnitude) -> juce::Colour {
            magnitude = juce::jlimit(0.0f, 1.0f, magnitude);
            juce::Colour c = juce::Colour(20, 22, 25);
            if (magnitude < 0.2f) return c.interpolatedWith(juce::Colours::darkred, magnitude * 5.0f);
            if (magnitude < 0.6f) return juce::Colours::darkred.interpolatedWith(juce::Colours::orange, (magnitude - 0.2f) / 0.4f);
            return juce::Colours::orange.interpolatedWith(juce::Colours::yellow, (magnitude - 0.6f) / 0.4f);
        };

        const int fftOrder = 10; // 1024 samples (512 Freq Bins)
        const int fftSize = 1 << fftOrder;
        const int hopSize = 256; // 4x Resolution (Overlap)
        
        juce::dsp::FFT fft(fftOrder);
        juce::dsp::WindowingFunction<float> window((size_t)fftSize, juce::dsp::WindowingFunction<float>::hann);
        
        int totalFrames = numSamples / hopSize;
        if (totalFrames > 0) {
            int imgHeight = fftSize / 2; 
            spectrogramImage = juce::Image(juce::Image::ARGB, totalFrames, imgHeight, true);
            juce::Image::BitmapData bmpData(spectrogramImage, juce::Image::BitmapData::writeOnly);
            
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
                    if (rawMag > 0.0001f) {
                       intensity = juce::jmin(1.0f, (float)std::log10(1.0f + rawMag * 50.0f) / 3.0f);
                    }
                    juce::Colour heat = getHeatColor(intensity);
                    bmpData.setPixelColour(frame, imgHeight - 1 - bin, heat);
                }
            }
        }
    }
};

struct MidiClipData {
    juce::String name;
    float startX = 0.0f;
    float width = 320.0f;
    float offsetX = 0.0f;
    bool isMuted = false;
    bool isSelected = false;
    std::vector<Note> notes;
    juce::Colour color;

    AutoLane autoVol;
    AutoLane autoPan;
    AutoLane autoPitch;
    AutoLane autoFilter;
};

// ============================================================
// DOUBLE BUFFERING (SNAPSHOTS) — Thread-Safe Audio Access
// ============================================================
// El audio thread solo lee snapshot. La UI modifica los OwnedArrays
// originales y luego llama commitSnapshot() para actualizar atómicamente.

// Metadatos de un AudioClip que el audio thread necesita.
// fileBufferPtr apunta al buffer original del OwnedArray — NUNCA se
// libera mientras el Track vive, por lo que es seguro desde el audio thread.
struct AudioClipSnapshot {
    float startX     = 0.0f;
    float width      = 0.0f;
    float offsetX    = 0.0f;
    bool  isMuted    = false;
    bool  isLoaded   = true; // Para carga asincrona
    const juce::AudioBuffer<float>* fileBufferPtr = nullptr; // solo lectura
    int   numChannels = 0;
    int   numSamples  = 0;
};

// Copia plana de una nota MIDI.
struct MidiNoteSnapshot {
    int   pitch = 0;
    int   x     = 0;
    int   width = 0;
};

// Metadatos de un MidiClip + notas para el audio thread.
struct MidiClipSnapshot {
    float startX  = 0.0f;
    float width   = 320.0f;
    float offsetX = 0.0f;
    bool  isMuted = false;
    std::vector<MidiNoteSnapshot> notes;
};

// Snapshot completo de los datos de reproducción de una pista.
// El audio thread lo adquiere con load(acquire) — wait-free, sin locks.
struct TrackSnapshot {
    std::vector<AudioClipSnapshot> audioClips;
    std::vector<MidiClipSnapshot>  midiClips;
    std::vector<MidiNoteSnapshot>  notes;        // notas directas (Piano Roll)
};

// ============================================================

class Track {
public:
    Track(int id, juce::String n, TrackType t) : trackId(id), name(n), type(t) {
        color = juce::Colour(juce::Random::getSystemRandom().nextFloat(), 0.6f, 0.8f, 1.0f);
    }

    ~Track() {
        // Liberar el snapshot actual si existe (el destructor corre en el UI thread)
        auto* old = snapshot.exchange(nullptr, std::memory_order_acq_rel);
        delete old;
    }

    int getId() const { return trackId; }
    juce::String getName() const { return name; }
    void setName(juce::String n) { name = n; }
    TrackType getType() const { return type; }
    juce::Colour getColor() const { return color; }
    void setColor(juce::Colour c) { color = c; }

    float getVolume() const { return volume; }
    void setVolume(float v) { volume = v; }
    float getBalance() const { return balance; }
    void setBalance(float b) { balance = b; }

    WaveformViewMode getWaveformViewMode() const { return waveformViewMode; }
    void setWaveformViewMode(WaveformViewMode mode) { waveformViewMode = mode; }

    juce::StringArray getPluginNames() const { return pluginNames; }
    void addPluginName(juce::String n) { pluginNames.add(n); }

    float getPreGain() const { return preGain; }
    void setPreGain(float v) { preGain = v; }

    float getPostGain() const { return postGain; }
    void setPostGain(float v) { postGain = v; }

    bool isPhaseInverted = false;
    bool isMonoActive = false;

    std::vector<Note> notes;

    juce::OwnedArray<BaseEffect> plugins;

    juce::OwnedArray<AudioClipData> audioClips;
    juce::OwnedArray<MidiClipData> midiClips;

    int folderDepthChange = 0;
    int folderDepth = 0;
    bool isCollapsed = false;
    bool isShowingInChildren = true;
    bool isFolderStart = false;
    bool isFolderEnd = false;

    bool isMuted = false;
    bool isSoloed = false;
    bool isSelected = false;
    bool isInlineEditingActive = false;

    juce::AudioBuffer<float> audioBuffer;
    juce::AudioBuffer<float> instrumentMixBuffer;
    juce::AudioBuffer<float> tempSynthBuffer;
    juce::AudioBuffer<float> midSideBuffer;
    
    // --- PDC (COMPENSACIÓN DE LATENCIA) ---
    juce::AudioBuffer<float> pdcBuffer; // Empieza en tamaño 0. Se aloca via allocatePdcBuffer()
    int pdcWritePtr = 0;
    int currentLatency = 0;
    int requiredDelay = 0;

    // Llamar desde el UI thread cuando se carga un plugin con latencia.
    // No hace nada si el buffer ya tiene el tamaño correcto (idempotente).
    void allocatePdcBuffer()
    {
        if (pdcBuffer.getNumSamples() >= 524288) return; // Ya alocado, no re-alocar
        pdcBuffer.setSize(2, 524288, false, true, false);
        pdcBuffer.clear();
        pdcWritePtr = 0;
    }

    float currentPeakLevelL = 0.0f;
    float currentPeakLevelR = 0.0f;

    SimpleLoudness preLoudness;
    SimpleLoudness postLoudness;
    bool isAnalyzersPrepared = false;

    // ============================================================
    // SNAPSHOT ATÓMICO — acceso wait-free desde el audio thread
    // Llamar commitSnapshot() desde el UI thread cada vez que se
    // añadan, eliminen o muevan clips/notas en este track.
    // ============================================================
    std::atomic<TrackSnapshot*> snapshot { nullptr };

    void commitSnapshot()
    {
        // Construir el nuevo snapshot en el heap (UI thread — aloja aquí, no en audio)
        auto* snap = new TrackSnapshot();

        // --- Audio clips: solo metadatos + puntero read-only al buffer ---
        snap->audioClips.reserve((size_t)audioClips.size());
        for (auto* ac : audioClips) {
            if (!ac) continue;
            AudioClipSnapshot acs;
            acs.startX        = ac->startX;
            acs.width         = ac->width;
            acs.offsetX       = ac->offsetX;
            acs.isMuted       = ac->isMuted;
            acs.isLoaded      = ac->isLoaded.load(std::memory_order_relaxed);
            acs.fileBufferPtr = &ac->fileBuffer;   // puntero de solo lectura, seguro
            acs.numChannels   = ac->fileBuffer.getNumChannels();
            acs.numSamples    = ac->fileBuffer.getNumSamples();
            snap->audioClips.push_back(acs);
        }

        // --- MIDI clips: metadatos + copia plana de notas ---
        snap->midiClips.reserve((size_t)midiClips.size());
        for (auto* mc : midiClips) {
            if (!mc) continue;
            MidiClipSnapshot mcs;
            mcs.startX  = mc->startX;
            mcs.width   = mc->width;
            mcs.offsetX = mc->offsetX;
            mcs.isMuted = mc->isMuted;
            mcs.notes.reserve(mc->notes.size());
            for (const auto& n : mc->notes) {
                mcs.notes.push_back({ n.pitch, n.x, n.width });
            }
            snap->midiClips.push_back(std::move(mcs));
        }

        // --- Notas directas (Piano Roll) ---
        snap->notes.reserve(notes.size());
        for (const auto& n : notes) {
            snap->notes.push_back({ n.pitch, n.x, n.width });
        }

        // Exchange atómico: el audio thread verá el nuevo snapshot inmediatamente.
        // El snapshot viejo se elimina en el Message Thread para no bloquear al audio.
        auto* old = snapshot.exchange(snap, std::memory_order_acq_rel);
        if (old) {
            juce::MessageManager::callAsync([old]() { delete old; });
        }
    }

private:
    int trackId;
    juce::String name;
    TrackType type;
    juce::Colour color;
    float volume = 0.8f;
    float balance = 0.0f;

    float preGain = 1.0f;
    float postGain = 1.0f;

    juce::StringArray pluginNames;

    WaveformViewMode waveformViewMode = WaveformViewMode::Combined;
};