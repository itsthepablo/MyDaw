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
            channels.add(new MixerChannelUI(t));
            contentComp.addAndMakeVisible(channels.getLast());
        }
    }
    resized();
}

void MixerComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(25, 28, 31));
}

void MixerComponent::resized() {
    viewport.setBounds(getLocalBounds());

    // === EL ALTO MANDA PARA EL ASPECT RATIO ===
    // 1. Vemos cuánto alto tenemos disponible en la pantalla
    int channelHeight = viewport.getHeight();

    // 2. Calculamos cuánto se ha encogido respecto a tu diseño original de 600
    float scale = (float)channelHeight / 600.0f;

    // 3. Calculamos el ancho PROPORCIONAL, para que NUNCA pierda tu Aspect Ratio (120 * escala)
    int channelWidth = juce::roundToInt(120.0f * scale);

    // 4. ¿Necesitamos barra horizontal?
    int totalWidth = channels.size() * channelWidth;
    if (totalWidth > viewport.getWidth()) {
        // La barra horizontal roba alto, así que recalculamos la escala matemática otra vez
        channelHeight -= viewport.getScrollBarThickness();
        scale = (float)channelHeight / 600.0f;
        channelWidth = juce::roundToInt(120.0f * scale);
        totalWidth = channels.size() * channelWidth;
    }

    contentComp.setSize(juce::jmax(viewport.getWidth(), totalWidth), channelHeight);

    // =========================================================================
    // FLEXBOX: Ahora dibuja con los tamaños proporcionales perfectos
    // =========================================================================
    juce::FlexBox fb;
    fb.flexDirection = juce::FlexBox::Direction::row;
    fb.flexWrap = juce::FlexBox::Wrap::noWrap;
    fb.alignItems = juce::FlexBox::AlignItems::flexStart;

    for (auto* channel : channels) {
        fb.items.add(juce::FlexItem(*channel)
            .withWidth((float)channelWidth)
            .withHeight((float)channelHeight)
            .withMargin(juce::FlexItem::Margin(0, 1, 0, 0)));
    }

    fb.performLayout(contentComp.getLocalBounds());
}