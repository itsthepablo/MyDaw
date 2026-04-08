#pragma once
#include <JuceHeader.h>
#include "../../Tracks/Track.h"

// ==============================================================================
// SLOTS Y RACKS (Componentes Auxiliares Aislados)
// ==============================================================================

class PluginSlot : public juce::Component {
public:
    PluginSlot(Track* t, int idx) : track(t), index(idx) {
        addAndMakeVisible(bypassBtn);
        bypassBtn.setButtonText("B");
        bypassBtn.setClickingTogglesState(true);
        bypassBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        bypassBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::dodgerblue);
        bypassBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::darkgrey);
        bypassBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
        bypassBtn.onClick = [this] { if (onBypassChanged) onBypassChanged(index, !bypassBtn.getToggleState()); };
    }

    void syncWithModel() {
        if (track && index < track->plugins.size()) {
            bool bypassed = track->plugins[index]->isBypassed();
            bypassBtn.setToggleState(!bypassed, juce::dontSendNotification);
        }
        repaint();
    }

    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().reduced(2);
        bool hasPlugin = track && index < track->plugins.size();
        bool bypassed = hasPlugin && track->plugins[index]->isBypassed();

        g.setColour(hasPlugin ? (bypassed ? juce::Colour(35, 35, 35) : juce::Colour(45, 50, 60)) : juce::Colour(25, 25, 25));
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

    void resized() override { bypassBtn.setBounds(4, 4, 16, getHeight() - 8); }

    void mouseDown(const juce::MouseEvent& e) override {
        if (bypassBtn.getBounds().contains(e.getPosition())) return;
        bool hasPlugin = track && index < track->plugins.size();
        if (hasPlugin) {
            if (e.mods.isPopupMenu()) {
                juce::PopupMenu m; m.addItem(1, "Eliminar");
                m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) { if (r == 1 && onDeletePlugin) onDeletePlugin(index); });
            } else if (onOpenPlugin) onOpenPlugin(index);
        } else {
            juce::PopupMenu m; m.addItem(1, "Native Utility"); m.addItem(2, "External VST3...");
            m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) {
                if (r == 1 && onAddNativeUtility) onAddNativeUtility(index);
                if (r == 2 && onAddVST3) onAddVST3(index);
            });
        }
    }

    std::function<void(int)> onOpenPlugin, onDeletePlugin, onAddNativeUtility, onAddVST3;
    std::function<void(int, bool)> onBypassChanged;

private:
    Track* track; int index; juce::TextButton bypassBtn;
};


class SendSlot : public juce::Component {
public:
    SendSlot(Track* t, int idx) : track(t), index(idx) {}
    
    void paint(juce::Graphics& g) override {
        auto b = getLocalBounds().reduced(2);
        bool hasSend = track && index < track->sends.size();
        g.setColour(hasSend ? juce::Colour(35, 60, 45) : juce::Colour(20, 25, 20));
        g.fillRoundedRectangle(b.toFloat(), 2.0f);
        if (hasSend) {
            g.setColour(juce::Colours::white.withAlpha(0.7f)); g.setFont(9.0f);
            g.drawText("SEND " + juce::String(index + 1), b.reduced(2), juce::Justification::centred);
        }
    }
    
    void mouseDown(const juce::MouseEvent& e) override {
        bool hasSend = track && index < track->sends.size();
        if (hasSend) {
            if (e.mods.isPopupMenu()) {
                juce::PopupMenu m; m.addItem(1, "Eliminar Envío");
                m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) { if (r == 1 && onDeleteSend) onDeleteSend(index); });
            }
        } else if (onAddSend) onAddSend();
    }
    
    std::function<void()> onAddSend; 
    std::function<void(int)> onDeleteSend;
private:
    Track* track; int index;
};