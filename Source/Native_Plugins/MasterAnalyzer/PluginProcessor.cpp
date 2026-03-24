#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
MiPrimerVSTAudioProcessor::MiPrimerVSTAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
    addParameter(modeSelector = new juce::AudioParameterChoice("mode", "Inspector Mode", modeOptions, 0));
    addParameter(btnLow = new juce::AudioParameterBool("low", "Low Band", false));
    addParameter(btnMid = new juce::AudioParameterBool("mid", "Mid Band", false));
    addParameter(btnHigh = new juce::AudioParameterBool("high", "High Band", false));

    addParameter(btnW = new juce::AudioParameterBool("w_func", "W Function", false));
    addParameter(btnPhase = new juce::AudioParameterBool("phase", "Phase Invert", false));
    addParameter(btnBypass = new juce::AudioParameterBool("bypass", "Knob Bypass", false));

    addParameter(btnFftMaster = new juce::AudioParameterBool("fft_master", "FFT Master Mode", false));
    addParameter(vuCalibParam = new juce::AudioParameterFloat("vu_calib", "VU Calibration", 0.0f, 24.0f, 5.0f));
}

MiPrimerVSTAudioProcessor::~MiPrimerVSTAudioProcessor() {}

// --- CORRECCIÓN: String nativo en vez de Macro de VST ---
const juce::String MiPrimerVSTAudioProcessor::getName() const { return "Master Analyzer"; }

bool MiPrimerVSTAudioProcessor::acceptsMidi() const { return false; }
bool MiPrimerVSTAudioProcessor::producesMidi() const { return false; }
bool MiPrimerVSTAudioProcessor::isMidiEffect() const { return false; }
double MiPrimerVSTAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int MiPrimerVSTAudioProcessor::getNumPrograms() { return 1; }
int MiPrimerVSTAudioProcessor::getCurrentProgram() { return 0; }
void MiPrimerVSTAudioProcessor::setCurrentProgram(int index) {}
const juce::String MiPrimerVSTAudioProcessor::getProgramName(int index) { return {}; }
void MiPrimerVSTAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

void MiPrimerVSTAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    for (int i = 0; i < 2; ++i) {
        lowBandFilter[i].reset(); highBandFilter[i].reset();
        midLowCut[i].reset(); midHighCut[i].reset();
    }

    auto lpCoeffs = juce::IIRCoefficients::makeLowPass(sampleRate, 250.0);
    auto hpCoeffs = juce::IIRCoefficients::makeHighPass(sampleRate, 4000.0);
    auto midLow = juce::IIRCoefficients::makeHighPass(sampleRate, 250.0);
    auto midHigh = juce::IIRCoefficients::makeLowPass(sampleRate, 4000.0);

    for (int i = 0; i < 2; ++i) {
        lowBandFilter[i].setCoefficients(lpCoeffs);
        highBandFilter[i].setCoefficients(hpCoeffs);
        midLowCut[i].setCoefficients(midLow);
        midHighCut[i].setCoefficients(midHigh);
    }

    meterL.prepare(sampleRate);
    meterR.prepare(sampleRate);

    vuDSP_L.prepare(sampleRate);
    vuDSP_R.prepare(sampleRate);

    spectrumAnalyzer.setup(12, sampleRate);

    std::fill(std::begin(scopeBufferL), std::end(scopeBufferL), 0.0f);
    std::fill(std::begin(scopeBufferR), std::end(scopeBufferR), 0.0f);

    lufsEngine.prepare(sampleRate, samplesPerBlock);
}

void MiPrimerVSTAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MiPrimerVSTAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) return false;
#endif
    return true;
}
#endif

void MiPrimerVSTAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) buffer.clear(i, 0, buffer.getNumSamples());
    if (buffer.getNumChannels() < 2) return;

    int currentMode = modeSelector->getIndex();
    bool isBypassed = btnBypass->get();

    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float inL = leftChannel[sample]; float inR = rightChannel[sample];

        if (!isBypassed)
        {
            switch (currentMode) {
            case 0: break;
            case 1: { float m = (inL + inR) * 0.5f; leftChannel[sample] = m; rightChannel[sample] = m; break; }
            case 2: { float s = (inL - inR) * 0.5f; leftChannel[sample] = s; rightChannel[sample] = -s; break; }
            case 3: leftChannel[sample] = -inL; rightChannel[sample] = -inR; break;
            case 4: leftChannel[sample] = inR; rightChannel[sample] = inL; break;
            case 5: { float sm = (inL - inR) * 0.5f; leftChannel[sample] = sm; rightChannel[sample] = sm; break; }
            case 6: leftChannel[sample] = inL; rightChannel[sample] = inL; break;
            case 7: leftChannel[sample] = inR; rightChannel[sample] = inR; break;
            }
        }
    }

    bool isLow = btnLow->get(); bool isMid = btnMid->get(); bool isHigh = btnHigh->get();
    if (isLow || isMid || isHigh) {
        for (int ch = 0; ch < 2; ++ch) {
            auto* data = buffer.getWritePointer(ch);
            if (isLow && !isMid && !isHigh) lowBandFilter[ch].processSamples(data, buffer.getNumSamples());
            else if (isHigh && !isLow && !isMid) highBandFilter[ch].processSamples(data, buffer.getNumSamples());
            else if (isMid) { midLowCut[ch].processSamples(data, buffer.getNumSamples()); midHighCut[ch].processSamples(data, buffer.getNumSamples()); }
        }
    }

    if (btnPhase->get()) {
        buffer.applyGain(1, 0, buffer.getNumSamples(), -1.0f);
    }

    spectrumAnalyzer.setMasterMode(btnFftMaster->get());

    spectrumAnalyzer.pushBuffer(buffer.getReadPointer(0), buffer.getNumSamples());

    vuDSP_L.setCalibration(vuCalibParam->get());
    vuDSP_R.setCalibration(vuCalibParam->get());

    if (buffer.getNumChannels() > 0) vuDSP_L.process(buffer.getReadPointer(0), buffer.getNumSamples());
    if (buffer.getNumChannels() > 1) vuDSP_R.process(buffer.getReadPointer(1), buffer.getNumSamples());
    else if (buffer.getNumChannels() > 0) vuDSP_R.process(buffer.getReadPointer(0), buffer.getNumSamples());

    vuLevelL.store(vuDSP_L.getVuValue());
    vuLevelR.store(vuDSP_R.getVuValue());
    vuPeakL.store(vuDSP_L.getPeakHoldVu());
    vuPeakR.store(vuDSP_R.getPeakHoldVu());

    lufsEngine.process(buffer);
    lufsShort = lufsEngine.getShortTerm();
    lufsIntegrated = lufsEngine.getIntegrated();
    lufsRange = lufsEngine.getRange();
    lufsTruePeak = lufsEngine.getTruePeak();

    float sumProduct = 0.0f;
    float sumSquares = 0.0f;
    const float* inL = buffer.getReadPointer(0);
    const float* inR = buffer.getReadPointer(1);

    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        sumProduct += inL[i] * inR[i];
        sumSquares += (inL[i] * inL[i]) + (inR[i] * inR[i]);
    }
    float corr = 0.0f;
    if (sumSquares > 0.00001f) corr = (2.0f * sumProduct) / sumSquares;
    float prevCorr = currentCorrelation.load();
    currentCorrelation.store(prevCorr * 0.95f + corr * 0.05f);

    meterL.process(buffer.getReadPointer(0), buffer.getNumSamples());
    meterR.process(buffer.getReadPointer(1), buffer.getNumSamples());

    rmsLevelLeft.store(meterL.getRMS());
    rmsLevelRight.store(meterR.getRMS());
    peakLevelLeft.store(meterL.getPeak());
    peakLevelRight.store(meterR.getPeak());

    const float* readL = buffer.getReadPointer(0);
    const float* readR = buffer.getReadPointer(1);
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        scopeBufferL[scopeWriteIndex] = readL[i];
        scopeBufferR[scopeWriteIndex] = readR[i];
        scopeWriteIndex++;
        if (scopeWriteIndex >= scopeSize) scopeWriteIndex = 0;
    }
}

bool MiPrimerVSTAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* MiPrimerVSTAudioProcessor::createEditor() { return new MiPrimerVSTAudioProcessorEditor(*this); }

void MiPrimerVSTAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::XmlElement xml("PLUGIN_STATE_V10");
    xml.setAttribute("mode", modeSelector->getIndex());
    xml.setAttribute("width", windowWidth);
    xml.setAttribute("height", windowHeight);

    xml.setAttribute("low", (int)btnLow->get());
    xml.setAttribute("mid", (int)btnMid->get());
    xml.setAttribute("high", (int)btnHigh->get());
    xml.setAttribute("w_func", (int)btnW->get());
    xml.setAttribute("phase", (int)btnPhase->get());
    xml.setAttribute("bypass", (int)btnBypass->get());

    xml.setAttribute("fft_master", (int)btnFftMaster->get());
    xml.setAttribute("vu_calib", vuCalibParam->get());

    xml.setAttribute("scopeColor", savedScopeColor.toString());
    xml.setAttribute("meterLow", savedMeterLow.toString());
    xml.setAttribute("meterMid", savedMeterMid.toString());
    xml.setAttribute("meterHigh", savedMeterHigh.toString());
    xml.setAttribute("corrPosColor", savedCorrelationPosColor.toString());
    xml.setAttribute("scopeMode", savedVectorscopeMode);

    xml.setAttribute("wX", wWindowX);
    xml.setAttribute("wY", wWindowY);
    xml.setAttribute("wW", wWindowWidth);
    xml.setAttribute("wH", wWindowHeight);

    copyXmlToBinary(xml, destData);
}

void MiPrimerVSTAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName("PLUGIN_STATE_V10"))
    {
        *modeSelector = xmlState->getIntAttribute("mode", 0);
        windowWidth = xmlState->getIntAttribute("width", 181);
        windowHeight = xmlState->getIntAttribute("height", 703);
        if (windowWidth < 100) windowWidth = 181;

        *btnLow = xmlState->getBoolAttribute("low", false);
        *btnMid = xmlState->getBoolAttribute("mid", false);
        *btnHigh = xmlState->getBoolAttribute("high", false);

        if (xmlState->hasAttribute("w_func")) *btnW = xmlState->getBoolAttribute("w_func", false);
        if (xmlState->hasAttribute("phase")) *btnPhase = xmlState->getBoolAttribute("phase", false);
        if (xmlState->hasAttribute("bypass")) *btnBypass = xmlState->getBoolAttribute("bypass", false);

        if (xmlState->hasAttribute("fft_master")) *btnFftMaster = xmlState->getBoolAttribute("fft_master", false);
        if (xmlState->hasAttribute("vu_calib")) *vuCalibParam = (float)xmlState->getDoubleAttribute("vu_calib", 5.0);

        if (xmlState->hasAttribute("scopeColor")) savedScopeColor = juce::Colour::fromString(xmlState->getStringAttribute("scopeColor"));
        if (xmlState->hasAttribute("meterLow")) savedMeterLow = juce::Colour::fromString(xmlState->getStringAttribute("meterLow"));
        if (xmlState->hasAttribute("meterMid")) savedMeterMid = juce::Colour::fromString(xmlState->getStringAttribute("meterMid"));
        if (xmlState->hasAttribute("meterHigh")) savedMeterHigh = juce::Colour::fromString(xmlState->getStringAttribute("meterHigh"));
        if (xmlState->hasAttribute("corrPosColor")) savedCorrelationPosColor = juce::Colour::fromString(xmlState->getStringAttribute("corrPosColor"));
        if (xmlState->hasAttribute("scopeMode")) savedVectorscopeMode = xmlState->getIntAttribute("scopeMode");

        wWindowX = xmlState->getIntAttribute("wX", -1);
        wWindowY = xmlState->getIntAttribute("wY", -1);
        wWindowWidth = xmlState->getIntAttribute("wW", 140);
        wWindowHeight = xmlState->getIntAttribute("wH", 140);
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MiPrimerVSTAudioProcessor(); }