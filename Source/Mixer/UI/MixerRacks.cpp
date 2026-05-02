#include "MixerRacks.h"
#include "../LookAndFeel/MixerColours.h"

// ==============================================================================
// PluginSlot Implementation
// ==============================================================================
PluginSlot::PluginSlot(Track* t, int idx) : track(t), index(idx) {
    addAndMakeVisible(bypassBtn);
    bypassBtn.setButtonText("B");
    bypassBtn.setClickingTogglesState(true);
    bypassBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    bypassBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::dodgerblue);
    bypassBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::darkgrey);
    bypassBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    bypassBtn.onClick = [this] { if (onBypassChanged) onBypassChanged(index, !bypassBtn.getToggleState()); };
}

void PluginSlot::syncWithModel() {
    if (track && index < track->plugins.size()) {
        bool bypassed = track->plugins[index]->isBypassed();
        bypassBtn.setToggleState(!bypassed, juce::dontSendNotification);
    }
    repaint();
}

void PluginSlot::paint(juce::Graphics& g) {
    auto b = getLocalBounds().reduced(2);
    bool hasPlugin = track && index < track->plugins.size();
    bool bypassed = hasPlugin && track->plugins[index]->isBypassed();

    auto activeCol = findColour(MixerColours::pluginActive);
    auto emptyCol = findColour(MixerColours::slotEmpty);

    g.setColour(hasPlugin ? (bypassed ? activeCol.darker(0.3f) : activeCol) : emptyCol);
    g.fillRoundedRectangle(b.toFloat(), 2.0f);
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.drawRoundedRectangle(b.toFloat(), 2.0f, 1.0f);

    if (hasPlugin) {
        auto* p = track->plugins[index];
        g.setColour(bypassed ? juce::Colours::grey : juce::Colours::white.withAlpha(0.8f));
        g.setFont(10.0f);
        g.drawText(p->getLoadedPluginName().substring(0, 20), b.reduced(2).withTrimmedLeft(20), juce::Justification::centredLeft);
        bypassBtn.setVisible(true);
    } else {
        bypassBtn.setVisible(false);
    }
}

void PluginSlot::resized() {
    bypassBtn.setBounds(4, 4, 16, getHeight() - 8);
}

void PluginSlot::mouseDown(const juce::MouseEvent& e) {
    if (bypassBtn.getBounds().contains(e.getPosition())) return;
    bool hasPlugin = track && index < track->plugins.size();
    if (hasPlugin) {
        if (e.mods.isPopupMenu()) {
            juce::PopupMenu m; m.addItem(1, "Eliminar");
            m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) { if (r == 1 && onDeletePlugin) onDeletePlugin(index); });
        } else if (onOpenPlugin) onOpenPlugin(index);
    } else {
        juce::PopupMenu m; m.addItem(1, "Native Utility"); m.addItem(2, "Native Node Patcher"); m.addItem(3, "External VST3...");
        m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) {
            if (r == 1 && onAddNativeUtility) onAddNativeUtility(index);
            if (r == 2 && onAddNativeNodePatcher) onAddNativeNodePatcher(index);
            if (r == 3 && onAddVST3) onAddVST3(index);
        });
    }
}

// ==============================================================================
// SendSlot Implementation
// ==============================================================================
SendSlot::SendSlot(Track* t, int idx) : track(t), index(idx) {}

void SendSlot::paint(juce::Graphics& g) {
    auto b = getLocalBounds().reduced(2);
    bool hasSend = track && index < track->routingData.sends.size();
    
    auto activeCol = findColour(MixerColours::sendActive);
    auto emptyCol = findColour(MixerColours::slotEmpty);

    g.setColour(hasSend ? activeCol : emptyCol);
    g.fillRoundedRectangle(b.toFloat(), 2.0f);
    if (hasSend) {
        g.setColour(juce::Colours::white.withAlpha(0.7f)); g.setFont(9.0f);
        g.drawText("SEND " + juce::String(index + 1), b.reduced(2), juce::Justification::centred);
    }
}

void SendSlot::mouseDown(const juce::MouseEvent& e) {
    bool hasSend = track && index < track->routingData.sends.size();
    if (hasSend) {
        if (e.mods.isPopupMenu()) {
            juce::PopupMenu m; m.addItem(1, "Eliminar Envío");
            m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) { if (r == 1 && onDeleteSend) onDeleteSend(index); });
        }
    } else if (onAddSend) onAddSend();
}
