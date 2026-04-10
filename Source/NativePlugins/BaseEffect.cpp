#include "BaseEffect.h"

// ==============================================================================
// BASE EFFECT IMPLEMENTATION
// ==============================================================================

BaseEffect::~BaseEffect() {}

void BaseEffect::setNonRealtime(bool isNonRealtime) {}

void BaseEffect::updatePlayHead(bool isPlaying, int64_t samplePos) {}

void BaseEffect::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, const juce::AudioBuffer<float>* sidechainBuffer) {
    processBlock(buffer, midiMessages);
}

bool BaseEffect::supportsSidechain() const {
    return false;
}

void BaseEffect::getStateInformation(juce::MemoryBlock& destData) {}

void BaseEffect::setStateInformation(const void* data, int sizeInBytes) {}

int BaseEffect::getLastKnownLatency() const {
    return lastKnownLatency;
}

void BaseEffect::setLastKnownLatency(int l) {
    lastKnownLatency = l;
}

PluginRouting BaseEffect::getRouting() const {
    return routing;
}

void BaseEffect::setRouting(PluginRouting r) {
    routing = r;
}

bool BaseEffect::isNative() const {
    return false;
}

juce::Component* BaseEffect::getCustomEditor() {
    return nullptr;
}

bool BaseEffect::getIsInstrument() const {
    return isInstrument;
}

void BaseEffect::setIsInstrument(bool isInst) {
    isInstrument = isInst;
}
