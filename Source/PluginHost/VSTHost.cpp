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
                                vstWindow->setVisible(true);
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
    if (vstPlugin != nullptr)
        vstPlugin->processBlock(buffer, midiMessages);
}

// AÑADIDO: Extrae el nombre del plugin cargado
juce::String VSTHost::getLoadedPluginName() const
{
    if (vstPlugin != nullptr) return vstPlugin->getName();
    return "VST";
}