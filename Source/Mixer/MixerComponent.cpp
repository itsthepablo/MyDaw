#include "MixerComponent.h"

MixerComponent::MixerComponent() {
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&contentComp, false);

    // Solo barra de scroll horizontal
    viewport.setScrollBarsShown(false, true, false, true);

    startTimerHz(10);
}

MixerComponent::~MixerComponent() {
    stopTimer();
}

void MixerComponent::timerCallback() {
    if (tracksRef != nullptr) {
        if (tracksRef->size() != channels.size()) {
            updateChannels();
        }
    }
}

void MixerComponent::updateChannels() {
    channels.clear();
    if (tracksRef != nullptr) {
        for (auto* t : *tracksRef) {
            auto* channel = new MixerChannelUI(t);
            
            // Vincular callbacks
            channel->onAddVST3 = onAddVST3;
            channel->onAddNativeUtility = onAddNativeUtility;
            channel->onOpenPlugin = onOpenPlugin;
            channel->onDeleteEffect = onDeleteEffect;
            channel->onBypassChanged = onBypassChanged;
            channel->onAddSend = onAddSend;
            channel->onDeleteSend = onDeleteSend;

            channels.add(channel);
            contentComp.addAndMakeVisible(channel);
        }
    }
    resized();
}

void MixerComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(25, 28, 31));
}

void MixerComponent::resized() {
    auto b = getLocalBounds();
    viewport.setBounds(b);

    // === EL ALTO MANDA PARA EL ASPECT RATIO ===
    int channelHeight = viewport.getHeight();
    float scale = (float)channelHeight / 600.0f;
    int channelWidth = juce::roundToInt(100.0f * scale);

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