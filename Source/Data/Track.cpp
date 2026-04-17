#include "Track.h"
#include "../Engine/Core/AudioEngine.h" 
#include "../PluginHost/VSTHost.h"

// ============================================================
// CONSTRUCTORES Y DESTRUCTOR
// ============================================================

Track::Track(int id, juce::String n, TrackType t) : trackId(id), name(n), type(t) 
{
    // Color Gris Claro estándar de DAW por defecto
    color = (t == TrackType::Folder) ? juce::Colour(60, 65, 75) : juce::Colour(150, 155, 160);
    
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

void Track::setName(juce::String n)
{
    if (name == n) return;
    name = n;
    
    // Inteligencia de Color - Solo si el usuario no ha puesto un color manual
    // o si el nombre está vacío (reset)
    if (n.isEmpty()) isColorManual = false;

    if (!isColorManual)
    {
        juce::Colour smartColor = SmartColorUtils::getColorForName(n);
        
        if (!smartColor.isTransparent())
        {
            setColor(smartColor, false);
        }
        else
        {
            // Revertir al gris estándar si no hay coincidencia inteligente
            setColor(juce::Colour(150, 155, 160), false);
        }
    }

    sendChangeMessage();
}

void Track::setColor(juce::Colour c, bool isManualAssignment)
{
    if (color == c) return;
    
    color = c;
    if (isManualAssignment) isColorManual = true;
    
    sendChangeMessage();
}

void Track::setType(TrackType newType)
{
    if (type == newType) return;
    type = newType;
    
    // Si se convierte en Folder y no tiene color manual, le damos el gris de folder
    if (!isColorManual && type == TrackType::Folder)
    {
        setColor(juce::Colour(60, 65, 75), false);
    }
    
    sendChangeMessage();
}

float Track::getModulationForTarget(const ModTarget& target, double beatPhase)
{
    if (target.type == ModTarget::None) return 0.0f;

    float total = 0.0f;
    for (auto* m : modulators)
    {
        // Bloqueo rápido para leer la lista de forma segura en el Audio Thread
        juce::ScopedLock sl(m->targetsLock);
        for (const auto& t : m->targets)
        {
            if (t == target)
            {
                total += m->getValueAt(beatPhase);
                break; // Un modulador solo aplica una vez por target
            }
        }
    }
    return total;
}

float Track::getModulationOffset(int parameterId, double beatPhase)
{
    ModTarget t;
    if (parameterId == 0) t.type = ModTarget::Volume;
    else if (parameterId == 1) t.type = ModTarget::Pan;
    
    return getModulationForTarget(t, beatPhase);
}

// ============================================================
// LÓGICA DE CLIPS Y DATOS
// ============================================================

AudioClip* Track::loadAndAddAudioClip(const juce::File& file, float startX)
{
    auto* clip = new AudioClip();
    clip->setStartX(startX);
    
    if (clip->loadFromFile(file, 44100.0)) // Valor base de sample rate
    {
        auto* thumb = new juce::AudioThumbnail(64, thumbnailFormatManager, thumbnailCache);
        thumb->reset(clip->getBuffer().getNumChannels(),
                    44100.0, // base rate
                    (juce::int64)clip->getBuffer().getNumSamples());
        thumb->addBlock(0, clip->getBuffer(), 0, clip->getBuffer().getNumSamples());
        
        clip->setThumbnail(std::unique_ptr<juce::AudioThumbnail>(thumb));
        
        audioClips.add(clip);
        commitSnapshot();
        return clip;
    }
    
    delete clip;
    return nullptr;
}

void Track::migrateMidiToRelative()
{
    bool changed = false;
    for (auto* clip : midiClips) {
        if (clip != nullptr) {
            clip->migrateToRelative();
            changed = true;
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
        if (ac != nullptr) {
        AudioClipSnapshot acs;
        acs.startX        = ac->getStartX();
        acs.width         = ac->getWidth();
        acs.offsetX       = ac->getOffsetX();
        acs.isMuted       = ac->getIsMuted();
        acs.isLoaded      = ac->isLoaded();
        acs.fileBufferPtr = &ac->getBuffer();   
        acs.numChannels   = ac->getBuffer().getNumChannels();
        acs.numSamples    = ac->getBuffer().getNumSamples();
        snap->audioClips.push_back(acs);
        }
    }

    snap->midiClips.reserve((size_t)midiClips.size());
    for (auto* mc : midiClips) {
        if (mc != nullptr) {
        MidiClipSnapshot mcs;
        mcs.startX  = mc->getStartX();
        mcs.width   = mc->getWidth();
        mcs.offsetX = mc->getOffsetX();
        mcs.isMuted = mc->getIsMuted();
        mcs.style   = mc->getStyle();
        mcs.notes.reserve(mc->getNotes().size());
        for (const auto& n : mc->getNotes()) {
            mcs.notes.push_back({ n.pitch, n.x, n.width, n.frequency });
        }
        snap->midiClips.push_back(std::move(mcs));
        }
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
