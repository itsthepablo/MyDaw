#include "MixerComponent.h"
#include "LookAndFeel/MixerColours.h"
#include "../Theme/CustomTheme.h"
#include <memory>

MixerComponent::MixerComponent() {
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&contentComp, false);

    // Solo barra de scroll horizontal
    viewport.setScrollBarsShown(false, true, false, true);

    startTimerHz(30);
}

MixerComponent::~MixerComponent() {
    stopTimer();
}

void MixerComponent::timerCallback() {
    if (!isShowing()) return;

    if (tracksRef != nullptr) {
        std::unique_ptr<juce::ScopedLock> lock;
        if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

        const int expectedChannels = tracksRef->size() + ((isMiniMixer && masterTrackPtr != nullptr) ? 1 : 0);
        
        if (expectedChannels != channels.size()) {
            updateChannels();
        }
    }

    if (!juce::ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown()) {
        for (auto* c : channels) {
            c->updateUI();
        }
    }
}

void MixerComponent::updateChannels() {
    std::unique_ptr<juce::ScopedLock> lock;
    if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

    channels.clear();

    if (isMiniMixer && masterTrackPtr != nullptr) {
        auto* masterChannel = new MixerChannelUI(masterTrackPtr, isMiniMixer);
        masterChannel->onOpenPlugin = onOpenPlugin;
        masterChannel->onDeleteEffect = onDeleteEffect;
        masterChannel->onBypassChanged = onBypassChanged;
        masterChannel->onAddVST3 = onAddVST3;
        masterChannel->onAddNativeUtility = onAddNativeUtility;
        masterChannel->onAddSend = onAddSend;
        masterChannel->onDeleteSend = onDeleteSend;
        masterChannel->onCreateAutomation = onCreateAutomation;
        
        channels.add(masterChannel);
        contentComp.addAndMakeVisible(masterChannel);
    }

    if (tracksRef != nullptr) {
        for (auto* t : *tracksRef) {
            auto* channel = new MixerChannelUI(t, isMiniMixer);
            channel->onAddVST3 = onAddVST3;
            channel->onAddNativeUtility = onAddNativeUtility;
            channel->onOpenPlugin = onOpenPlugin;
            channel->onDeleteEffect = onDeleteEffect;
            channel->onBypassChanged = onBypassChanged;
            channel->onAddSend = onAddSend;
            channel->onDeleteSend = onDeleteSend;
            channel->onCreateAutomation = onCreateAutomation;

            channels.add(channel);
            contentComp.addAndMakeVisible(channel);
        }
    }
    resized();
}

void MixerComponent::paint(juce::Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(MixerColours::mainBackground));
}

void MixerComponent::resized() {
    auto b = getLocalBounds();
    viewport.setBounds(b);

    int channelHeight = viewport.getHeight();
    float scale = (float)channelHeight / 600.0f;
    
    int baseWidth = isMiniMixer ? 130 : 100;
    int channelWidth = juce::roundToInt(baseWidth * scale);

    int totalWidth = channels.size() * (channelWidth + 1);
    if (totalWidth > viewport.getWidth()) {
        channelHeight -= viewport.getScrollBarThickness();
        totalWidth = channels.size() * (channelWidth + 1);
    }

    contentComp.setSize(juce::jmax(viewport.getWidth(), totalWidth), channelHeight);

    for (int i = 0; i < channels.size(); ++i) {
        channels[i]->setBounds(i * (channelWidth + 1), 0, channelWidth, channelHeight);
    }
}
