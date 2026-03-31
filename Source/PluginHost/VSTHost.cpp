#include "VSTHost.h"

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
            
            // Usamos un bloque de samples prudente (512) para la inicialización
            vstPlugin = formatManager.createPluginInstance(desc, sampleRate, 512, errorMessage);

            if (vstPlugin != nullptr)
            {
                pluginPath = path;
                vstPlugin->setPlayHead(&playHead);

                // --- CONFIGURACIÓN DE BUSES (RESTABLECER ESTADO ORIGINAL) ---
                // No borramos lógica, restauramos la configuración granular de buses
                juce::AudioProcessor::BusesLayout layout = vstPlugin->getBusesLayout();
                
                // Configurar Bus Principal a Stereo si es posible
                if (layout.inputBuses.size() > 0)
                    layout.inputBuses.getReference(0) = juce::AudioChannelSet::stereo();
                if (layout.outputBuses.size() > 0)
                    layout.outputBuses.getReference(0) = juce::AudioChannelSet::stereo();

                // Activar Bus de Sidechain (Input 1) a Stereo si existe
                if (layout.inputBuses.size() > 1) {
                    layout.inputBuses.getReference(1) = juce::AudioChannelSet::stereo();
                }

                // Intentar aplicar el layout
                if (!vstPlugin->setBusesLayout(layout)) {
                    // Fallback a Mono-Stereo dinámico (algunos plugins antiguos)
                    layout = vstPlugin->getBusesLayout();
                }

                vstPlugin->enableAllBuses();
                
                // Reiniciar para asegurar que el plugin procese el nuevo layout
                vstPlugin->releaseResources();
                vstPlugin->prepareToPlay(sampleRate, 512);

                vstWindow = std::make_unique<VSTWindow>(vstPlugin.get());
                callback(true);
                return;
            }
        }
    }
    callback(false);
}

bool VSTHost::isLoaded() const
{
    return vstPlugin != nullptr;
}

void VSTHost::showWindow()
{
    if (vstWindow != nullptr)
    {
        vstWindow->setVisible(true);
        vstWindow->toFront(true);
    }
}

void VSTHost::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    if (vstPlugin != nullptr) {
        // Reservar buffer de fallback con margen de seguridad (evita allocations en runtime)
        int maxInputChans = vstPlugin->getTotalNumInputChannels();
        int maxOutputChans = vstPlugin->getTotalNumOutputChannels();
        int totalChansNeeded = juce::jmax(maxInputChans, maxOutputChans, 2);
        
        fallbackBuffer.setSize(totalChansNeeded, juce::jmax(8192, maximumExpectedSamplesPerBlock));
        vstPlugin->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
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
    if (vstPlugin != nullptr && !bypassed) {
        const int numSamples = buffer.getNumSamples();
        const int inCh = vstPlugin->getTotalNumInputChannels();
        const int outCh = vstPlugin->getTotalNumOutputChannels();
        const int totalChans = juce::jmax(inCh, outCh);

        // --- DETECCIÓN DE SIDECHAIN ACTIVO ---
        bool hasSidechainBus = false;
        int sidechainBusInputOffset = 0;
        
        if (vstPlugin->getBusCount(true) > 1) {
            auto* scBus = vstPlugin->getBus(true, 1);
            if (scBus != nullptr && scBus->isEnabled()) {
                hasSidechainBus = true;
                sidechainBusInputOffset = vstPlugin->getBus(true, 0)->getNumberOfChannels();
            }
        }

        // --- CASO 1: PROCESAMIENTO CON SIDECHAIN ---
        if (sidechainBuffer != nullptr && hasSidechainBus) {
            // Limpiar y preparar buffer de trabajo interno (SIN ALLOCATIONS)
            for (int i = 0; i < totalChans; ++i) fallbackBuffer.clear(i, 0, numSamples);

            // Copiar entrada principal (Normalmente canales 0-1)
            int mainInCh = vstPlugin->getBus(true, 0)->getNumberOfChannels();
            int chansToCopyMain = juce::jmin(buffer.getNumChannels(), mainInCh);
            for (int i = 0; i < chansToCopyMain; ++i)
                fallbackBuffer.copyFrom(i, 0, buffer, i, 0, numSamples);

            // Copiar sidechain al offset del bus secundario
            int scChToCopy = juce::jmin(sidechainBuffer->getNumChannels(), vstPlugin->getBus(true, 1)->getNumberOfChannels());
            for (int i = 0; i < scChToCopy; ++i)
                fallbackBuffer.copyFrom(sidechainBusInputOffset + i, 0, *sidechainBuffer, i, 0, numSamples);

            juce::AudioBuffer<float> proxyBuffer(fallbackBuffer.getArrayOfWritePointers(), totalChans, numSamples);
            vstPlugin->processBlock(proxyBuffer, midiMessages);

            // Mapear resultado de vuelta (Stereo Out)
            int chansToRestore = juce::jmin(buffer.getNumChannels(), outCh);
            for (int i = 0; i < chansToRestore; ++i)
                buffer.copyFrom(i, 0, proxyBuffer, i, 0, numSamples);
        }
        // --- CASO 2: MAPEO DE CANALES (Si el VST necesita más canales que el host) ---
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
        // --- CASO 3: FAST-PATH (Canales directos) ---
        else {
            int safeChans = juce::jmin(totalChans, buffer.getNumChannels());
            if (safeChans > 0) {
                juce::AudioBuffer<float> proxyBuffer(buffer.getArrayOfWritePointers(), safeChans, numSamples);
                vstPlugin->processBlock(proxyBuffer, midiMessages);
            }
        }

        // Limpieza de canales residuales para evitar ruido
        for (int ch = outCh; ch < buffer.getNumChannels(); ++ch) {
            buffer.clear(ch, 0, numSamples);
        }
    }
}

juce::String VSTHost::getLoadedPluginName() const
{
    if (vstPlugin != nullptr) return vstPlugin->getName();
    return "VST";
}

int VSTHost::getLatencySamples() const
{
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
    if (vstPlugin == nullptr) return false;
    return vstPlugin->getBusCount(true) > 1;
}