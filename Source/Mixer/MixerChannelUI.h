#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include "../UI/LevelMeter.h"
#include "LookAndFeel/MixerLookAndFeel.h" // <-- Aquí conectamos la estética aislada

// ==============================================================================
// 3. SLOTS DE EFECTOS Y ENVÍOS INTERACTIVOS (Componentes auxiliares)
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
        }
        else {
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
            }
            else if (onOpenPlugin) onOpenPlugin(index);
        }
        else {
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
        }
        else if (onAddSend) onAddSend();
    }
    std::function<void()> onAddSend; std::function<void(int)> onDeleteSend;
private:
    Track* track; int index;
};

// ==============================================================================
// 4. CANAL PRINCIPAL DEL MIXER (Raks Escroleables)
// ==============================================================================
class MixerChannelUI : public juce::Component {
public:
    class FXRack : public juce::Component {
    public:
        FXRack(Track* t, MixerChannelUI* p) : track(t), parent(p) {}
        void syncSlots() {
            int needed = std::max(10, (int)track->plugins.size() + 1);
            while (slots.size() < needed) {
                auto* s = new PluginSlot(track, slots.size());
                s->onOpenPlugin = [this](int idx) { if (parent->onOpenPlugin) parent->onOpenPlugin(*track, idx); };
                s->onBypassChanged = [this](int idx, bool b) { if (parent->onBypassChanged) parent->onBypassChanged(*track, idx, b); };
                s->onAddNativeUtility = [this](int idx) { if (parent->onAddNativeUtility) parent->onAddNativeUtility(*track); };
                s->onAddVST3 = [this](int idx) { if (parent->onAddVST3) parent->onAddVST3(*track); };
                s->onDeletePlugin = [this](int idx) { if (parent->onDeleteEffect) parent->onDeleteEffect(*track, idx); };
                slots.add(s);
                addAndMakeVisible(s);
            }
            while (slots.size() > needed) slots.remove(slots.size() - 1);
            for (auto* s : slots) s->syncWithModel();
            resized();
        }
        void resized() override {
            int h = 18;
            for (int i = 0; i < slots.size(); ++i) slots[i]->setBounds(0, i * h, getWidth(), h);
            setSize(getWidth(), slots.size() * h);
        }
    private:
        Track* track; MixerChannelUI* parent; juce::OwnedArray<PluginSlot> slots;
    };

    class SendRack : public juce::Component {
    public:
        SendRack(Track* t, MixerChannelUI* p) : track(t), parent(p) {}
        void syncSlots() {
            int needed = std::max(4, (int)track->sends.size() + 1);
            while (slots.size() < needed) {
                auto* s = new SendSlot(track, slots.size());
                s->onAddSend = [this] { if (parent->onAddSend) parent->onAddSend(*track); };
                s->onDeleteSend = [this](int idx) { if (parent->onDeleteSend) parent->onDeleteSend(*track, idx); };
                slots.add(s);
                addAndMakeVisible(s);
            }
            while (slots.size() > needed) slots.remove(slots.size() - 1);
            resized();
        }
        void resized() override {
            int h = 18;
            for (int i = 0; i < slots.size(); ++i) slots[i]->setBounds(0, i * h, getWidth(), h);
            setSize(getWidth(), slots.size() * h);
        }
    private:
        Track* track; MixerChannelUI* parent; juce::OwnedArray<SendSlot> slots;
    };

    MixerChannelUI(Track* t, bool miniMode = false) : track(t), meter(t), isMiniMode(miniMode), fxRack(t, this), sendRack(t, this) {
        setLookAndFeel(&mixerLAF);

        if (!isMiniMode) {
            addAndMakeVisible(fxViewport);
            fxViewport.setViewedComponent(&fxRack);
            fxViewport.setScrollBarsShown(true, false, true, false);
            fxViewport.setScrollBarThickness(6);

            addAndMakeVisible(sendViewport);
            sendViewport.setViewedComponent(&sendRack);
            sendViewport.setScrollBarsShown(true, false, true, false);
            sendViewport.setScrollBarThickness(6);
        }

        // --- Paneo (Normal) ---
        panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panKnob.setRange(-1.0, 1.0);
        panKnob.onValueChange = [this] { track->setBalance((float)panKnob.getValue()); };
        addAndMakeVisible(panKnob);

        // --- Paneo Dual (Solo para Modo Principal) ---
        if (!isMiniMode) {
            panToggle.setButtonText("NORM");
            panToggle.setClickingTogglesState(true);
            panToggle.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkgrey);
            panToggle.onClick = [this] {
                bool dual = panToggle.getToggleState();
                panToggle.setButtonText(dual ? "DUAL" : "NORM");
                track->panningModeDual.store(dual);
                updatePanVisibility();
                };
            addAndMakeVisible(panToggle);

            panL.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            panL.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            panL.setRange(-1.0, 1.0);
            panL.onValueChange = [this] { track->panL.store((float)panL.getValue()); };
            addChildComponent(panL);

            panR.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            panR.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
            panR.setRange(-1.0, 1.0);
            panR.onValueChange = [this] { track->panR.store((float)panR.getValue()); };
            addChildComponent(panR);
        }

        // --- Fader y Meter ---
        addAndMakeVisible(meter);
        fader.setSliderStyle(juce::Slider::LinearVertical);
        fader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
        fader.setRange(0.0, 1.5);
        fader.onValueChange = [this] { track->setVolume((float)fader.getValue()); };
        addAndMakeVisible(fader);

        setupButton(muteBtn, "M", juce::Colours::red);
        muteBtn.onClick = [this] { track->isMuted = muteBtn.getToggleState(); };
        setupButton(soloBtn, "S", juce::Colours::yellow);
        soloBtn.onClick = [this] { track->isSoloed = soloBtn.getToggleState(); };
        setupButton(phaseBtn, "PHS", juce::Colours::cyan);
        phaseBtn.onClick = [this] { track->isPhaseInverted = phaseBtn.getToggleState(); };
        setupButton(recBtn, "R", juce::Colours::red.brighter());

        trackName.setJustificationType(juce::Justification::centred);
        trackName.setFont(juce::Font(13.0f, juce::Font::bold));
        addAndMakeVisible(trackName);

        fader.addMouseListener(this, false);
        panKnob.addMouseListener(this, false);
        if (!isMiniMode) {
            panL.addMouseListener(this, false);
            panR.addMouseListener(this, false);
        }

        updateUI();
    }

    void updateUI() {
        fader.setValue(track->getVolume(), juce::dontSendNotification);
        panKnob.setValue(track->getBalance(), juce::dontSendNotification);

        if (!isMiniMode) {
            bool dual = track->panningModeDual.load();
            bool uiDual = panToggle.getToggleState();
            if (dual != uiDual) {
                panToggle.setToggleState(dual, juce::dontSendNotification);
                panToggle.setButtonText(dual ? "DUAL" : "NORM");
                updatePanVisibility();
            }
            panL.setValue(track->panL.load(), juce::dontSendNotification);
            panR.setValue(track->panR.load(), juce::dontSendNotification);
        }

        muteBtn.setToggleState(track->isMuted, juce::dontSendNotification);
        soloBtn.setToggleState(track->isSoloed, juce::dontSendNotification);
        phaseBtn.setToggleState(track->isPhaseInverted, juce::dontSendNotification);
        if (trackName.getText() != track->getName()) trackName.setText(track->getName(), juce::dontSendNotification);
        if (!isMiniMode) {
            fxRack.syncSlots();
            sendRack.syncSlots();
        }
        repaint();
    }

    ~MixerChannelUI() override { setLookAndFeel(nullptr); }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(35, 38, 42));
        g.setColour(juce::Colours::black.withAlpha(0.3f)); g.drawRect(getLocalBounds(), 1);
        g.setColour(track->getColor()); g.fillRect(0, getHeight() - 5, getWidth(), 5);
    }

    void resized() override {
        auto b = getLocalBounds().reduced(4);

        if (!isMiniMode) {
            auto topArea = b.removeFromTop(100);
            auto panToggleArea = topArea.removeFromTop(20);
            panToggle.setBounds(panToggleArea.withSizeKeepingCentre(40, 18));

            if (track->panningModeDual.load()) {
                panL.setBounds(topArea.removeFromLeft(topArea.getWidth() / 2).reduced(5));
                panR.setBounds(topArea.reduced(5));
            }
            else {
                panKnob.setBounds(topArea.withSizeKeepingCentre(45, 45));
            }

            auto fxArea = b.removeFromTop(180);
            fxViewport.setBounds(fxArea);
            fxRack.setBounds(0, 0, fxViewport.getMaximumVisibleWidth(), fxRack.getHeight());

            b.removeFromTop(5);
            auto sendArea = b.removeFromTop(72);
            sendViewport.setBounds(sendArea);
            sendRack.setBounds(0, 0, sendViewport.getMaximumVisibleWidth(), sendRack.getHeight());
            b.removeFromTop(5);
        }
        else {
            auto topArea = b.removeFromTop(60);
            panKnob.setBounds(topArea.withSizeKeepingCentre(45, 45));
            fxViewport.setVisible(false);
            sendViewport.setVisible(false);
        }

        auto btnArea = b.removeFromTop(25);
        auto btnW = btnArea.getWidth() / 4;
        muteBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(1));
        soloBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(1));
        phaseBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(1));
        recBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(1));

        trackName.setBounds(b.removeFromBottom(20));
        meter.setBounds(b.removeFromRight(isMiniMode ? 20 : 22));
        b.removeFromRight(3);
        fader.setBounds(b);
    }

    std::function<void(Track&)> onAddVST3, onAddNativeUtility, onAddSend;
    std::function<void(Track&, int)> onOpenPlugin, onDeleteEffect, onDeleteSend;
    std::function<void(Track&, int, bool)> onBypassChanged;
    std::function<void(Track&, int, juce::String)> onCreateAutomation;

    void mouseDown(const juce::MouseEvent& e) override {
        if (e.mods.isPopupMenu()) {
            if (e.originalComponent == &fader) {
                showAutomationMenu(0, "Volume");
            }
            else if (e.originalComponent == &panKnob || e.originalComponent == &panL || e.originalComponent == &panR) {
                showAutomationMenu(1, "Pan");
            }
        }
    }

private:
    void showAutomationMenu(int paramId, const juce::String& paramName) {
        juce::PopupMenu m;
        m.addItem(1, "Create Automation Clip");
        m.showMenuAsync(juce::PopupMenu::Options(), [this, paramId, paramName](int res) {
            if (res == 1 && onCreateAutomation) {
                onCreateAutomation(*track, paramId, paramName);
            }
        });
    }

    void setupButton(juce::TextButton& btn, juce::String text, juce::Colour onCol) {
        btn.setButtonText(text); btn.setClickingTogglesState(true);
        btn.setColour(juce::TextButton::buttonOnColourId, onCol);
        btn.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
        addAndMakeVisible(btn);
    }

    void updatePanVisibility() {
        if (isMiniMode) return;
        bool dual = track->panningModeDual.load();
        panKnob.setVisible(!dual);
        panL.setVisible(dual);
        panR.setVisible(dual);
        resized();
    }

    bool isMiniMode; Track* track; MixerLookAndFeel mixerLAF;
    juce::TextButton panToggle;
    juce::Slider panKnob, panL, panR, fader; LevelMeter meter;
    juce::TextButton muteBtn, soloBtn, phaseBtn, recBtn; juce::Label trackName;
    
    juce::Viewport fxViewport, sendViewport;
    FXRack fxRack;
    SendRack sendRack;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerChannelUI)
};