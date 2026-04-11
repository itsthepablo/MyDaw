#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
TpAnalyzerAudioProcessor::TpAnalyzerAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true))
#endif
{
}

TpAnalyzerAudioProcessor::~TpAnalyzerAudioProcessor()
{
}

//==============================================================================
const juce::String TpAnalyzerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TpAnalyzerAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool TpAnalyzerAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool TpAnalyzerAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double TpAnalyzerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TpAnalyzerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int TpAnalyzerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void TpAnalyzerAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String TpAnalyzerAudioProcessor::getProgramName(int index)
{
    return {};
}

void TpAnalyzerAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void TpAnalyzerAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Inicializamos el analizador (Orden 12 = 4096 muestras, tipo SPAN)
    analyzer.setup(12, sampleRate);
}

void TpAnalyzerAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TpAnalyzerAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void TpAnalyzerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // --- ENVIAR AUDIO AL ANALIZADOR ---
    // Enviamos el canal 0 (Izquierdo).
    if (buffer.getNumChannels() > 0)
    {
        analyzer.pushBuffer(buffer.getReadPointer(0), buffer.getNumSamples());
    }
}

//==============================================================================
bool TpAnalyzerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* TpAnalyzerAudioProcessor::createEditor()
{
    return new TpAnalyzerAudioProcessorEditor(*this);
}

//==============================================================================
void TpAnalyzerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
}

void TpAnalyzerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TpAnalyzerAudioProcessor();
}