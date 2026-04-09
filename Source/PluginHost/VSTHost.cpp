#include "../PluginHost/VSTHost.h"
#include "../Tracks/TrackContainer.h"
#include "../Engine/Core/AudioEngine.h"
#include "../Engine/Routing/RoutingMatrix.h"

// ==============================================================================
// VST CUSTOM HEADER IMPLEMENTATION
// ==============================================================================
VSTCustomHeader::VSTCustomHeader(juce::DocumentWindow* w, BaseEffect* fx, TrackContainer* container, std::function<void()> onUpdate)
    : window(w), effect(fx), trackContainer(container), onTopologyUpdate(onUpdate)
{
    addAndMakeVisible(closeBtn);
    closeBtn.setButtonText("x");
    closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(150, 40, 40));
    closeBtn.onClick = [this] { window->closeButtonPressed(); };

    if (effect && effect->supportsSidechain() && trackContainer)
    {
        addAndMakeVisible(scSelector);
        
        // --- DATA BRIDGE ---
        auto refreshTracks = [this] {
            juce::Array<std::pair<int, juce::String>> trackData;
            for (auto* t : trackContainer->getTracks())
                trackData.add({ t->getId(), t->getName() });
            
            scSelector.updateAvailableTracks(trackData, effect->sidechain.sourceTrackId.load());
        };

        refreshTracks();

        scSelector.onSourceChanged = [this, refreshTracks](int selectedId) {
            effect->sidechain.sourceTrackId.store(selectedId);
            if (effect->onSidechainChanged) effect->onSidechainChanged();
            
            // --- ACTUALIZAR TOPOLOGÍA DE AUDIO ---
            if (onTopologyUpdate) onTopologyUpdate();
        };

        trackContainer->onTracksReordered = [refreshTracks] { refreshTracks(); };
        trackContainer->onTrackAdded = [refreshTracks] { refreshTracks(); };
    }
}

void VSTCustomHeader::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(20, 22, 25));
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(15.0f, juce::Font::bold));
    
    // Si hay selector de sidechain, movemos el título a la izquierda
    if (scSelector.isVisible())
        g.drawText(window->getName(), 10, 0, getWidth() / 2, getHeight(), juce::Justification::centredLeft);
    else
        g.drawText(window->getName(), getLocalBounds(), juce::Justification::centred);
}

void VSTCustomHeader::resized()
{
    auto area = getLocalBounds();
    closeBtn.setBounds(area.removeFromRight(30).reduced(5));
    
    if (scSelector.isVisible())
        scSelector.setBounds(area.removeFromRight(180));
}

// ==============================================================================
// VST HOST IMPLEMENTATION
// ==============================================================================
VSTHost::VSTHost()
{
    formatManager.addDefaultFormats();
}

VSTHost::~VSTHost()
{
    vstWindow.reset();
    vstPlugin.reset();
}

void VSTHost::loadPluginAsync(double sampleRate, std::function<void(bool)> callback)
{
    fileChooser = std::make_unique<juce::FileChooser>("Selecciona un VST3", juce::File(), "*.vst3");
    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(flags, [this, sampleRate, callback](const juce::FileChooser& fc)
        {
            juce::File pluginFile = fc.getResult();
            if (pluginFile.existsAsFile())
                loadPluginFromPath(pluginFile.getFullPathName(), sampleRate, callback);
            else
                callback(false);
        });
}

void VSTHost::loadPluginFromPath(const juce::String& path, double sampleRate, std::function<void(bool)> callback)
{
    juce::File pluginFile(path);
    if (!pluginFile.existsAsFile()) {
        callback(false);
        return;
    }

    juce::AudioPluginFormat* vst3Format = nullptr;
    for (int i = 0; i < formatManager.getNumFormats(); ++i) {
        if (formatManager.getFormat(i)->getName() == "VST3") {
            vst3Format = formatManager.getFormat(i);
            break;
        }
    }

    if (vst3Format != nullptr && vst3Format->fileMightContainThisPluginType(pluginFile.getFullPathName()))
    {
        juce::OwnedArray<juce::PluginDescription> typesFound;
        vst3Format->findAllTypesForFile(typesFound, pluginFile.getFullPathName());

        if (typesFound.size() > 0)
        {
            juce::PluginDescription desc = *typesFound[0];
            juce::String errorMessage;
            
            isInitializing = true;
            
            // --- BLINDAJE DE CREACIÓN ---
            // Usamos un bloque de samples prudente (512) y un SR estándar (44100)
            // para la inicialización, independientemente del motor, para evitar crashes.
            double safeRate = sampleRate > 8000.0 ? sampleRate : 44100.0;
            vstPlugin = formatManager.createPluginInstance(desc, safeRate, 512, errorMessage);

            if (vstPlugin != nullptr)
            {
                pluginPath = path;
                vstPlugin->setPlayHead(&playHead);

                // --- CONFIGURACIÓN DE BUSES SEGURA EN CUARENTENA ---
                juce::AudioProcessor::BusesLayout layout = vstPlugin->getBusesLayout();
                if (layout.inputBuses.size() > 0)
                    layout.inputBuses.getReference(0) = juce::AudioChannelSet::stereo();
                if (layout.outputBuses.size() > 0)
                    layout.outputBuses.getReference(0) = juce::AudioChannelSet::stereo();

                vstPlugin->setBusesLayout(layout);
                vstPlugin->enableAllBuses();
                
                refreshSidechainSupport(); // Cachear una vez estable
                
                isInitializing = false; // FIN DE CUARENTENA
                callback(true);
                return;
            }
            isInitializing = false;
        }
    }
    callback(false);
}

bool VSTHost::isLoaded() const
{
    return vstPlugin != nullptr;
}

void VSTHost::showWindow(TrackContainer* container)
{
    if (vstPlugin != nullptr)
    {
        // Si no hay ventana o el container cambió (necesario para refrescar el selector), la recreamos
        if (vstWindow == nullptr)
        {
            vstWindow = std::make_unique<VSTWindow>(vstPlugin.get(), this, container, onTopologyUpdate);
        }
        
        vstWindow->setVisible(true);
        vstWindow->toFront(true);
    }
}

void VSTHost::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    if (vstPlugin != nullptr) {
        // --- BLINDAJE DE SEGURIDAD ---
        // Forzar un sample rate válido para evitar crashes inmediatos
        double safeRate = sampleRate > 0.0 ? sampleRate : 44100.0;
        int safeBlock = maximumExpectedSamplesPerBlock > 0 ? maximumExpectedSamplesPerBlock : 512;

        // Pre-asignar siempre un margen de canales generoso (mínimo 16)
        // Algunos plugins (iZotope, Cableguys) pueden cambiar dinámicamente de canales.
        int maxInputChans = vstPlugin->getTotalNumInputChannels();
        int maxOutputChans = vstPlugin->getTotalNumOutputChannels();
        int totalChansNeeded = juce::jmax(maxInputChans, maxOutputChans, 16);
        
        fallbackBuffer.setSize(totalChansNeeded, juce::jmax(8192, safeBlock));
        vstPlugin->prepareToPlay(safeRate, safeBlock);
        
        refreshSidechainSupport(); // Re-validar tras preparar
    }
}

void VSTHost::reset()
{
    if (vstPlugin != nullptr)
        vstPlugin->reset();
}

void VSTHost::setNonRealtime(bool isNonRealtime)
{
    if (vstPlugin != nullptr)
        vstPlugin->setNonRealtime(isNonRealtime);
}

void VSTHost::updatePlayHead(bool isPlaying, int64_t samplePos)
{
    playHead.isPlaying = isPlaying;
    playHead.currentSample = samplePos;
}

void VSTHost::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    processBlock(buffer, midiMessages, nullptr);
}

void VSTHost::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, const juce::AudioBuffer<float>* sidechainBuffer)
{
    if (isInitializing) {
        buffer.clear();
        return;
    }

    if (vstPlugin != nullptr && !bypassed) {
        const int numSamples = buffer.getNumSamples();
        const int inCh = vstPlugin->getTotalNumInputChannels();
        const int outCh = vstPlugin->getTotalNumOutputChannels();
        const int totalChans = juce::jmax(inCh, outCh);

        bool hasSidechainBus = false;
        int sidechainBusInputOffset = 0;
        
        if (vstPlugin->getBusCount(true) > 1) {
            auto* scBus = vstPlugin->getBus(true, 1);
            if (scBus != nullptr && scBus->isEnabled()) {
                hasSidechainBus = true;
                sidechainBusInputOffset = vstPlugin->getBus(true, 0)->getNumberOfChannels();
            }
        }

        if (sidechainBuffer != nullptr && hasSidechainBus) {
            // --- GUARDA DE SEGURIDAD (ANTI-CRASH) ---
            if (totalChans > fallbackBuffer.getNumChannels()) {
                // Si llegamos aquí, el plugin ha cambiado de buses en caliente más allá de nuestra reserva.
                // Redimensionamos para salvar el DAW del crash.
                fallbackBuffer.setSize(totalChans + 4, numSamples, true, true);
            }

            for (int i = 0; i < juce::jmin(totalChans, fallbackBuffer.getNumChannels()); ++i) 
                fallbackBuffer.clear(i, 0, numSamples);

            int mainInCh = vstPlugin->getBus(true, 0)->getNumberOfChannels();
            int chansToCopyMain = juce::jmin(buffer.getNumChannels(), mainInCh);
            for (int i = 0; i < chansToCopyMain; ++i)
                fallbackBuffer.copyFrom(i, 0, buffer, i, 0, numSamples);

            int scChToCopy = juce::jmin(sidechainBuffer->getNumChannels(), vstPlugin->getBus(true, 1)->getNumberOfChannels());
            for (int i = 0; i < scChToCopy; ++i)
                fallbackBuffer.copyFrom(sidechainBusInputOffset + i, 0, *sidechainBuffer, i, 0, numSamples);

            juce::AudioBuffer<float> proxyBuffer(fallbackBuffer.getArrayOfWritePointers(), totalChans, numSamples);
            vstPlugin->processBlock(proxyBuffer, midiMessages);

            int chansToRestore = juce::jmin(buffer.getNumChannels(), outCh);
            for (int i = 0; i < chansToRestore; ++i)
                buffer.copyFrom(i, 0, proxyBuffer, i, 0, numSamples);
        }
        else if (totalChans > buffer.getNumChannels()) {
            if (numSamples <= fallbackBuffer.getNumSamples() && totalChans <= fallbackBuffer.getNumChannels()) {
                for (int i = 0; i < totalChans; ++i) fallbackBuffer.clear(i, 0, numSamples);
                
                int chansToCopy = juce::jmin(buffer.getNumChannels(), totalChans);
                for (int i = 0; i < chansToCopy; ++i)
                    fallbackBuffer.copyFrom(i, 0, buffer, i, 0, numSamples);
                
                juce::AudioBuffer<float> proxyBuffer(fallbackBuffer.getArrayOfWritePointers(), totalChans, numSamples);
                vstPlugin->processBlock(proxyBuffer, midiMessages);

                int chansToRestore = juce::jmin(buffer.getNumChannels(), outCh);
                for (int i = 0; i < chansToRestore; ++i)
                    buffer.copyFrom(i, 0, proxyBuffer, i, 0, numSamples);
            }
        }
        else {
            int safeChans = juce::jmin(totalChans, buffer.getNumChannels());
            if (safeChans > 0) {
                juce::AudioBuffer<float> proxyBuffer(buffer.getArrayOfWritePointers(), safeChans, numSamples);
                vstPlugin->processBlock(proxyBuffer, midiMessages);
            }
        }

        for (int ch = outCh; ch < buffer.getNumChannels(); ++ch) {
            buffer.clear(ch, 0, numSamples);
        }
    }
}

juce::String VSTHost::getLoadedPluginName() const
{
    if (isInitializing) return "Cargando...";
    if (vstPlugin != nullptr) return vstPlugin->getName();
    return "VST";
}

int VSTHost::getLatencySamples() const
{
    if (isInitializing) return 0;
    if (vstPlugin != nullptr) {
        return vstPlugin->getLatencySamples();
    }
    return 0;
}

void VSTHost::getStateInformation(juce::MemoryBlock& destData)
{
    if (vstPlugin != nullptr)
        vstPlugin->getStateInformation(destData);
}

void VSTHost::setStateInformation(const void* data, int sizeInBytes)
{
    if (vstPlugin != nullptr)
        vstPlugin->setStateInformation(data, sizeInBytes);
}

bool VSTHost::supportsSidechain() const
{
    return sidechainSupported;
}

void VSTHost::refreshSidechainSupport()
{
    if (vstPlugin != nullptr) {
        sidechainSupported = (vstPlugin->getBusCount(true) > 1);
    } else {
        sidechainSupported = false;
    }
}