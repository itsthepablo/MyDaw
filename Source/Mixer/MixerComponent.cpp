#include "MixerComponent.h"

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
        // Corregir comparación: channels ya incluye el Master (si existe)
        const int expectedChannels = tracksRef->size() + (masterTrackPtr != nullptr ? 1 : 0);
        
        if (expectedChannels != channels.size()) {
            updateChannels();
        }
    }

    // Sincronizar valores de UI con el modelo SOLO si el usuario no esta
    // interactuando con ningun control (evita sobreescribir faders/knobs
    // mientras se arrastra con el mouse).
    if (!juce::ModifierKeys::getCurrentModifiers().isAnyMouseButtonDown()) {
        for (auto* c : channels) {
            c->updateUI();
        }
    }
}

void MixerComponent::updateChannels() {
    channels.clear();

    // 1. PRIMERO EL CANAL MASTER (si existe)
    if (masterTrackPtr != nullptr) {
        auto* masterChannel = new MixerChannelUI(masterTrackPtr, isMiniMixer);
        // El master no suele tener sends o borrado en el mezclador, pero vinculamos los básicos
        masterChannel->onOpenPlugin = onOpenPlugin;
        masterChannel->onDeleteEffect = onDeleteEffect;
        masterChannel->onBypassChanged = onBypassChanged;
        
        channels.add(masterChannel);
        contentComp.addAndMakeVisible(masterChannel);
    }

    // 2. LUEGO LOS TRACKS DEL PROYECTO
    if (tracksRef != nullptr) {
        for (auto* t : *tracksRef) {
            auto* channel = new MixerChannelUI(t, isMiniMixer);
            
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
    
    // Si es mini mixer, usamos una base más ancha (130 en lugar de 100)
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
