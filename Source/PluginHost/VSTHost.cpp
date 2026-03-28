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

void VSTHost::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (vstPlugin != nullptr && !bypassed) {
        // ==============================================================================
        // FIX COMERCIAL: BLINDAJE ESTÉREO
        // Creamos un sub-búfer que "engaña" al VST3 para que solo vea 2 canales.
        // Esto evita que Pro-Q 3 active Sidechains fantasma y se coma el audio original.
        // ==============================================================================
        int safeChans = juce::jmin(2, buffer.getNumChannels());

        if (safeChans > 0) {
            juce::AudioBuffer<float> stereoBuffer(buffer.getArrayOfWritePointers(), safeChans, buffer.getNumSamples());
            vstPlugin->processBlock(stereoBuffer, midiMessages);
        }

        // Destruimos cualquier basura residual en los canales 3 al 16 
        for (int ch = safeChans; ch < buffer.getNumChannels(); ++ch) {
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