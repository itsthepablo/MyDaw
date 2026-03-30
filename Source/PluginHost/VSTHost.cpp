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
            {
                juce::AudioPluginFormat* vst3Format = nullptr;
                for (int i = 0; i < formatManager.getNumFormats(); ++i) {
                    if (formatManager.getFormat(i)->getName() == "VST3") {
                        vst3Format = formatManager.getFormat(i);
                        break;
                    }
                }

                if (vst3Format != nullptr)
                {
                    juce::PluginDescription desc;

                    if (vst3Format->fileMightContainThisPluginType(pluginFile.getFullPathName()))
                    {
                        juce::OwnedArray<juce::PluginDescription> typesFound;
                        vst3Format->findAllTypesForFile(typesFound, pluginFile.getFullPathName());

                        if (typesFound.size() > 0)
                        {
                            desc = *typesFound[0];
                            juce::String errorMessage;

                            vstPlugin = formatManager.createPluginInstance(desc, sampleRate, 512, errorMessage);

                            if (vstPlugin != nullptr)
                            {
                                // ==============================================================================
                                // LA DICTADURA DEL VST3: Sometiendo a FabFilter y plugins rebeldes
                                // ==============================================================================

                                // 1. Inyectamos el reloj maestro para que el FFT rojo se mueva a tiempo
                                vstPlugin->setPlayHead(&playHead);

                                // 2. Obtenemos la arquitectura completa que el VST3 quiere usar por defecto
                                juce::AudioProcessor::BusesLayout layout = vstPlugin->getBusesLayout();

                                // 3. Obligamos al Bus 0 (El Principal) a ser Est	reo puro
                                if (layout.inputBuses.size() > 0)
                                    layout.inputBuses.getReference(0) = juce::AudioChannelSet::stereo();
                                if (layout.outputBuses.size() > 0)
                                    layout.outputBuses.getReference(0) = juce::AudioChannelSet::stereo();

                                // 4. EL TIRO DE GRACIA AL ESPECTRO GRIS: 
                                // Marcamos cualquier otro Bus (Sidechain, Surround) como INEXISTENTE.
                                for (int i = 1; i < layout.inputBuses.size(); ++i)
                                    layout.inputBuses.getReference(i) = juce::AudioChannelSet::disabled();

                                for (int i = 1; i < layout.outputBuses.size(); ++i)
                                    layout.outputBuses.getReference(i) = juce::AudioChannelSet::disabled();

                                // 5. Le inyectamos esta nueva realidad al plugin a la fuerza
                                vstPlugin->setBusesLayout(layout);

                                // 6. Nos aseguramos de que JUCE respete esta decisin
                                vstPlugin->enableAllBuses();

                                vstWindow = std::make_unique<VSTWindow>(vstPlugin.get());
                                callback(true);
                                return;
                            }
                            else
                            {
                                juce::Logger::writeToLog("Error cargando VST3: " + errorMessage);
                            }
                        }
                    }
                }
            }
            callback(false);
        });
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
    if (vstPlugin != nullptr)
        vstPlugin->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
}

void VSTHost::reset()
{
    if (vstPlugin != nullptr)
        vstPlugin->reset();
}

void VSTHost::updatePlayHead(bool isPlaying, int64_t samplePos)
{
    playHead.isPlaying = isPlaying;
    playHead.currentSample = samplePos;
}

void VSTHost::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (vstPlugin != nullptr && !bypassed) {
        // Evaluamos cuántos canales reales (matemáticos) exige el VST3
        int inCh = vstPlugin->getTotalNumInputChannels();
        int outCh = vstPlugin->getTotalNumOutputChannels();
        int totalChans = juce::jmax(inCh, outCh);

        if (totalChans > buffer.getNumChannels()) {
            // --- FALLBACK DE SEGURIDAD (ANTI OUT-OF-BOUNDS CRASH) ---
            // Plugins (The God Particle, FabFilter, etc.) exigen buses extra o mueren.
            juce::AudioBuffer<float> bigBuffer(totalChans, buffer.getNumSamples());
            bigBuffer.clear();
            
            // Inyectamos nuestro audio
            int chansToCopy = juce::jmin(buffer.getNumChannels(), totalChans);
            for (int i = 0; i < chansToCopy; ++i)
                bigBuffer.copyFrom(i, 0, buffer, i, 0, buffer.getNumSamples());
            
            vstPlugin->processBlock(bigBuffer, midiMessages);

            // Recuperamos su respuesta
            for (int i = 0; i < chansToCopy; ++i)
                buffer.copyFrom(i, 0, bigBuffer, i, 0, buffer.getNumSamples());
        } 
        else {
            // --- FAST-PATH: El plugin exige igual o menos canales ---
            int safeChans = juce::jmax(1, juce::jmin(totalChans, buffer.getNumChannels()));
            if (safeChans > 0) {
                juce::AudioBuffer<float> safeBuffer(buffer.getArrayOfWritePointers(), safeChans, buffer.getNumSamples());
                vstPlugin->processBlock(safeBuffer, midiMessages);
            }
        }

        // El inodoro del Sidechain (Matamos basura residual de buses que el VST3 no reescribió)
        int activeChans = juce::jmax(2, totalChans);
        for (int ch = activeChans; ch < buffer.getNumChannels(); ++ch) {
            buffer.clear(ch, 0, buffer.getNumSamples());
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