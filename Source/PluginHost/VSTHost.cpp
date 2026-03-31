#include "VSTHost.h"
#include "../Tracks/TrackContainer.h"

// ==============================================================================
// VST CUSTOM HEADER IMPLEMENTATION
// ==============================================================================
VSTCustomHeader::VSTCustomHeader(juce::DocumentWindow* w, BaseEffect* fx, TrackContainer* container)
    : window(w), effect(fx), trackContainer(container)
{
    addAndMakeVisible(closeBtn);
    closeBtn.setButtonText("x");
    closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(150, 40, 40));
    closeBtn.onClick = [this] { window->closeButtonPressed(); };

    if (effect && effect->supportsSidechain() && trackContainer)
    {
        addAndMakeVisible(sidechainLabel);
        sidechainLabel.setText("Sidechain:", juce::dontSendNotification);
        sidechainLabel.setFont(juce::Font(11.0f));
        sidechainLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));

        addAndMakeVisible(sidechainSelector);
        sidechainSelector.addItem("None", 1000); // ID especial para desactivar
        
        const auto& tracks = trackContainer->getTracks();
        for (int i = 0; i < tracks.size(); ++i)
        {
            // Evitar que una pista se use a sí misma como sidechain (Feedback loop)
            // Nota: Aquí necesitaríamos saber en qué pista está el plugin. 
            // Por simplicidad, listamos todas y el motor gestionará la seguridad.
            sidechainSelector.addItem(tracks[i]->getName(), tracks[i]->getId());
        }

        int currentSc = effect->sidechainSourceTrackId.load();
        if (currentSc == -1) sidechainSelector.setSelectedId(1000, juce::dontSendNotification);
        else sidechainSelector.setSelectedId(currentSc, juce::dontSendNotification);

        sidechainSelector.onChange = [this] {
            int selectedId = sidechainSelector.getSelectedId();
            if (selectedId == 1000) effect->sidechainSourceTrackId.store(-1);
            else effect->sidechainSourceTrackId.store(selectedId);
            
            if (effect->onSidechainChanged) effect->onSidechainChanged();
        };
    }
}

void VSTCustomHeader::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(20, 22, 25));
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font(15.0f, juce::Font::bold));
    
    // Si hay selector de sidechain, movemos el título a la izquierda
    if (sidechainSelector.isVisible())
        g.drawText(window->getName(), 10, 0, getWidth() / 2, getHeight(), juce::Justification::centredLeft);
    else
        g.drawText(window->getName(), getLocalBounds(), juce::Justification::centred);
}

void VSTCustomHeader::resized()
{
    int bs = getHeight() - 8;
    closeBtn.setBounds(getWidth() - bs - 8, 4, bs, bs);

    if (sidechainSelector.isVisible())
    {
        int comboW = 120;
        sidechainSelector.setBounds(getWidth() - bs - 20 - comboW, 5, comboW, getHeight() - 10);
        sidechainLabel.setBounds(sidechainSelector.getX() - 65, 0, 60, getHeight());
    }
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
            
            // Usamos un bloque de samples prudente (512) para la inicialización
            vstPlugin = formatManager.createPluginInstance(desc, sampleRate, 512, errorMessage);

            if (vstPlugin != nullptr)
            {
                pluginPath = path;
                vstPlugin->setPlayHead(&playHead);

                // --- CONFIGURACIÓN DE BUSES (RESTABLECER ESTADO ORIGINAL) ---
                juce::AudioProcessor::BusesLayout layout = vstPlugin->getBusesLayout();
                
                if (layout.inputBuses.size() > 0)
                    layout.inputBuses.getReference(0) = juce::AudioChannelSet::stereo();
                if (layout.outputBuses.size() > 0)
                    layout.outputBuses.getReference(0) = juce::AudioChannelSet::stereo();

                if (layout.inputBuses.size() > 1) {
                    layout.inputBuses.getReference(1) = juce::AudioChannelSet::stereo();
                }

                if (!vstPlugin->setBusesLayout(layout)) {
                    layout = vstPlugin->getBusesLayout();
                }

                vstPlugin->enableAllBuses();
                vstPlugin->releaseResources();
                vstPlugin->prepareToPlay(sampleRate, 512);

                // Eliminada la creación inmediata de la ventana para que showWindow la cree con el TrackContainer correcto
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

void VSTHost::showWindow(TrackContainer* container)
{
    if (vstPlugin != nullptr)
    {
        // Si no hay ventana o el container cambió (necesario para refrescar el selector), la recreamos
        if (vstWindow == nullptr)
        {
            vstWindow = std::make_unique<VSTWindow>(vstPlugin.get(), this, container);
        }
        
        vstWindow->setVisible(true);
        vstWindow->toFront(true);
    }
}

void VSTHost::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    if (vstPlugin != nullptr) {
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
            for (int i = 0; i < totalChans; ++i) fallbackBuffer.clear(i, 0, numSamples);

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