#include "Track.h"
#include "../Engine/Core/AudioEngine.h" 
#include "../PluginHost/VSTHost.h"

// ============================================================
// CONSTRUCTORES Y DESTRUCTOR
// ============================================================

Track::Track(int id, juce::String n, TrackType t) : trackId(id), name(n), type(t) 
{
    color = juce::Colour(juce::Random::getSystemRandom().nextFloat(), 0.6f, 0.8f, 1.0f);
    if (t == TrackType::Folder) color = juce::Colour(60, 65, 75);
    thumbnailFormatManager.registerBasicFormats();
}

Track::~Track() 
{
    auto* old = snapshot.exchange(nullptr, std::memory_order_acq_rel);
    delete old;
}

// ============================================================
// MÉTODOS DE CONFIGURACIÓN
// ============================================================

void Track::prepare(double sampleRate, int samplesPerBlock)
{
    mixerData.prepare(sampleRate, samplesPerBlock);
    dsp.prepare(sampleRate, samplesPerBlock);
}

// ============================================================
// LÓGICA DE CLIPS Y DATOS
// ============================================================

AudioClipData* Track::loadAndAddAudioClip(const juce::File& file, float startX)
{
    juce::AudioFormatManager manager;
    manager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(manager.createReaderFor(file));

    if (reader != nullptr)
    {
        auto* clip = new AudioClipData();
        clip->name = file.getFileNameWithoutExtension();
        clip->sourceFilePath = file.getFullPathName();
        clip->startX = startX;
        
        double duration = (double)reader->lengthInSamples / reader->sampleRate;
        clip->width = (float)(duration * 160.0); 
        clip->originalWidth = clip->width;
        clip->sourceSampleRate = reader->sampleRate;

        clip->fileBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&clip->fileBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

        clip->generateCache();

        clip->thumbnail = std::make_unique<juce::AudioThumbnail>(64, thumbnailFormatManager, thumbnailCache);
        clip->thumbnail->reset(clip->fileBuffer.getNumChannels(),
                               clip->sourceSampleRate,
                               (juce::int64)clip->fileBuffer.getNumSamples());
        clip->thumbnail->addBlock(0, clip->fileBuffer, 0, clip->fileBuffer.getNumSamples());

        audioClips.add(clip);
        commitSnapshot();
        return clip;
    }
    return nullptr;
}

void Track::migrateMidiToRelative()
{
    bool changed = false;
    for (auto* clip : midiClips) {
        if (!clip) continue;
        for (auto& note : clip->notes) {
            if (note.x >= (int)clip->startX && note.x < (int)(clip->startX + clip->width + 1.0f)) {
                note.x -= (int)clip->startX;
                changed = true;
            }
        }
    }
    if (changed) commitSnapshot();
}

// ============================================================
// SISTEMA DE SNAPSHOTS (THREAD-SAFETY REAL-TIME)
// ============================================================

void Track::commitSnapshot()
{
    // PDC: delegate allocation to dsp
    if (!audioClips.isEmpty() || !midiClips.isEmpty() || !plugins.isEmpty() || !notes.empty())
        dsp.allocatePdcBuffer();

    auto* snap = new TrackSnapshot();

    snap->audioClips.reserve((size_t)audioClips.size());
    for (auto* ac : audioClips) {
        if (!ac) continue;
        AudioClipSnapshot acs;
        acs.startX        = ac->startX;
        acs.width         = ac->width;
        acs.offsetX       = ac->offsetX;
        acs.isMuted       = ac->isMuted;
        acs.isLoaded      = ac->isLoaded.load(std::memory_order_relaxed);
        acs.fileBufferPtr = &ac->fileBuffer;   
        acs.numChannels   = ac->fileBuffer.getNumChannels();
        acs.numSamples    = ac->fileBuffer.getNumSamples();
        snap->audioClips.push_back(acs);
    }

    snap->midiClips.reserve((size_t)midiClips.size());
    for (auto* mc : midiClips) {
        if (!mc) continue;
        MidiClipSnapshot mcs;
        mcs.startX  = mc->startX;
        mcs.width   = mc->width;
        mcs.offsetX = mc->offsetX;
        mcs.isMuted = mc->isMuted;
        mcs.style   = mc->style;
        mcs.notes.reserve(mc->notes.size());
        for (const auto& n : mc->notes) {
            mcs.notes.push_back({ n.pitch, n.x, n.width, n.frequency });
        }
        snap->midiClips.push_back(std::move(mcs));
    }

    snap->notes.reserve(notes.size());
    for (const auto& n : notes) {
        snap->notes.push_back({ n.pitch, n.x, n.width, n.frequency });
    }

    snap->automations.reserve(automationClips.size());
    for (auto* a : automationClips) {
        if (!a) continue;
        AutomationClipSnapshot as;
        as.parameterId = a->parameterId;
        as.lane = a->lane;      
        snap->automations.push_back(std::move(as));
    }

    auto* old = snapshot.exchange(snap, std::memory_order_acq_rel);
    if (old) {
        juce::MessageManager::callAsync([old]() { delete old; });
    }
}
