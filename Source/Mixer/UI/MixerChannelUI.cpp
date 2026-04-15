#include "MixerChannelUI.h"
#include "../LookAndFeel/MixerColours.h"

// --- FXRack Implementation ---
MixerChannelUI::FXRack::FXRack(Track* t, MixerChannelUI* p) : track(t), parent(p) {}

void MixerChannelUI::FXRack::syncSlots() {
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

void MixerChannelUI::FXRack::resized() {
    int h = 18;
    for (int i = 0; i < slots.size(); ++i) slots[i]->setBounds(0, i * h, getWidth(), h);
    setSize(getWidth(), slots.size() * h);
}

// --- SendRack Implementation ---
MixerChannelUI::SendRack::SendRack(Track* t, MixerChannelUI* p) : track(t), parent(p) {}

void MixerChannelUI::SendRack::syncSlots() {
    int needed = std::max(4, (int)track->routingData.sends.size() + 1);
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

void MixerChannelUI::SendRack::resized() {
    int h = 18;
    for (int i = 0; i < slots.size(); ++i) slots[i]->setBounds(0, i * h, getWidth(), h);
    setSize(getWidth(), slots.size() * h);
}

// --- MixerChannelUI Implementation ---
MixerChannelUI::MixerChannelUI(Track* t, bool miniMode) 
    : track(t), meter(t), isMiniMode(miniMode), fxRack(t, this), sendRack(t, this) 
{
    meterLF.setTrackColour(track->getColor());
    meter.setLookAndFeel(&meterLF);
    midMeter.setLookAndFeel(&meterLF);
    sideMeter.setLookAndFeel(&meterLF);
    startTimerHz(60);
    
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

    panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panKnob.setRange(-1.0, 1.0);
    panKnob.onValueChange = [this] { track->setBalance((float)panKnob.getValue()); };
    addAndMakeVisible(panKnob);

    if (!isMiniMode) {
        panToggle.setButtonText("NORM");
        panToggle.setClickingTogglesState(true);
        panToggle.setColour(juce::TextButton::buttonOnColourId, juce::Colours::darkgrey);
        panToggle.onClick = [this] {
            bool dual = panToggle.getToggleState();
            panToggle.setButtonText(dual ? "DUAL" : "NORM");
            MixerParameterBridge::setPanningModeDual(track, dual);
            updatePanVisibility();
        };
        addAndMakeVisible(panToggle);

        panL.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panL.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panL.setRange(-1.0, 1.0);
        panL.onValueChange = [this] { MixerParameterBridge::setPanL(track, (float)panL.getValue()); };
        addChildComponent(panL);

        panR.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        panR.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        panR.setRange(-1.0, 1.0);
        panR.onValueChange = [this] { MixerParameterBridge::setPanR(track, (float)panR.getValue()); };
        addChildComponent(panR);
    }

    // --- HELPERS PARA VISUALIZACIÓN (MODO dB) ---
    auto dbToText = [](double v) { 
        if (v <= -63.5) return juce::String("-inf dB");
        return (v > 0.0 ? "+" : "") + juce::String(v, 1) + " dB";
    };
    auto textToDb = [](const juce::String& t) { 
        if (t.containsIgnoreCase("-inf")) return -64.0;
        return t.getDoubleValue(); 
    };

    addAndMakeVisible(meter);
    
    // Master Fader (Escala Pro 80/20)
    fader.setSliderStyle(juce::Slider::LinearVertical);
    fader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    fader.setRange(-64.0, 16.0); // 0dB al 80% exacto (64/80 = 0.8)
    fader.textFromValueFunction = dbToText;
    fader.valueFromTextFunction = textToDb;
    fader.onValueChange = [this] { 
        float gain = (fader.getValue() <= -63.5) ? 0.0f : juce::Decibels::decibelsToGain((float)fader.getValue());
        track->setVolume(gain); 
    };
    addAndMakeVisible(fader);

    setupButton(muteBtn, "M", juce::Colours::red);
    muteBtn.onClick = [this] { MixerParameterBridge::setMuted(track, muteBtn.getToggleState()); };
    setupButton(soloBtn, "S", juce::Colours::yellow);
    soloBtn.onClick = [this] { MixerParameterBridge::setSoloed(track, soloBtn.getToggleState()); };
    
    setupButton(midSoloBtn, "MID", juce::Colours::blue.withAlpha(0.6f));
    midSoloBtn.onClick = [this] { 
        MixerParameterBridge::setMidSolo(track, midSoloBtn.getToggleState()); 
        updateUI(); // Sincronizar exclusividad
    };
    
    setupButton(sideSoloBtn, "SID", juce::Colours::purple.withAlpha(0.6f));
    sideSoloBtn.onClick = [this] { 
        MixerParameterBridge::setSideSolo(track, sideSoloBtn.getToggleState()); 
        updateUI(); // Sincronizar exclusividad
    };

    setupButton(phaseBtn, "PHS", juce::Colours::cyan);
    phaseBtn.onClick = [this] { MixerParameterBridge::setPhaseInverted(track, phaseBtn.getToggleState()); };
    setupButton(recBtn, "R", juce::Colours::red.brighter());

    // --- MID-SIDE COMPONENTS ---
    msToggle.setButtonText("M/S");
    msToggle.setClickingTogglesState(true);
    msToggle.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange.withAlpha(0.8f));
    msToggle.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    msToggle.onClick = [this] { 
        MixerParameterBridge::setMidSideMode(track, msToggle.getToggleState());
        updateUI(); // Esto disparará resized()
    };
    addAndMakeVisible(msToggle);

    midFader.setSliderStyle(juce::Slider::LinearVertical);
    midFader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    midFader.setRange(-64.0, 16.0);
    midFader.textFromValueFunction = dbToText;
    midFader.valueFromTextFunction = textToDb;
    midFader.onValueChange = [this] { 
        float gain = (midFader.getValue() <= -63.5) ? 0.0f : juce::Decibels::decibelsToGain((float)midFader.getValue());
        MixerParameterBridge::setMidVolume(track, gain); 
    };
    addChildComponent(midFader);

    sideFader.setSliderStyle(juce::Slider::LinearVertical);
    sideFader.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 15);
    sideFader.setRange(-64.0, 16.0);
    sideFader.textFromValueFunction = dbToText;
    sideFader.valueFromTextFunction = textToDb;
    sideFader.onValueChange = [this] { 
        float gain = (sideFader.getValue() <= -63.5) ? 0.0f : juce::Decibels::decibelsToGain((float)sideFader.getValue());
        MixerParameterBridge::setSideVolume(track, gain); 
    };
    addChildComponent(sideFader);

    midMeter.setTrack(track);
    midMeter.setSource(LevelMeter::Mid);
    addChildComponent(midMeter);

    sideMeter.setTrack(track);
    sideMeter.setSource(LevelMeter::Side);
    addChildComponent(sideMeter);

    midLabel.setText("MID", juce::dontSendNotification);
    midLabel.setJustificationType(juce::Justification::centred);
    midLabel.setFont(juce::Font(10.0f));
    addChildComponent(midLabel);

    sideLabel.setText("SIDE", juce::dontSendNotification);
    sideLabel.setJustificationType(juce::Justification::centred);
    sideLabel.setFont(juce::Font(10.0f));
    addChildComponent(sideLabel);

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

MixerChannelUI::~MixerChannelUI() {
    stopTimer();
    meter.setLookAndFeel(nullptr);
    midMeter.setLookAndFeel(nullptr);
    sideMeter.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void MixerChannelUI::updateUI() {
    fader.setValue(track->getVolume(), juce::dontSendNotification);
    panKnob.setValue(track->getBalance(), juce::dontSendNotification);

    // --- ASIGNAR MOD TARGETS (Solo una vez) ---
    if (!fader.modTarget.isValid()) {
        fader.modTarget.type = ModTarget::Volume;
        panKnob.modTarget.type = ModTarget::Pan;
        if (!isMiniMode) {
            panL.modTarget.type = ModTarget::Pan; // O dual pan si se prefiere
            panR.modTarget.type = ModTarget::Pan;
        }
    }

    if (!isMiniMode) {
        bool dual = MixerParameterBridge::isPanningModeDual(track);
        bool uiDual = panToggle.getToggleState();
        if (dual != uiDual) {
            panToggle.setToggleState(dual, juce::dontSendNotification);
            panToggle.setButtonText(dual ? "DUAL" : "NORM");
            updatePanVisibility();
        }
        panL.setValue(MixerParameterBridge::getPanL(track), juce::dontSendNotification);
        panR.setValue(MixerParameterBridge::getPanR(track), juce::dontSendNotification);
    }

    muteBtn.setToggleState(MixerParameterBridge::isMuted(track), juce::dontSendNotification);
    soloBtn.setToggleState(MixerParameterBridge::isSoloed(track), juce::dontSendNotification);
    midSoloBtn.setToggleState(MixerParameterBridge::isMidSolo(track), juce::dontSendNotification);
    sideSoloBtn.setToggleState(MixerParameterBridge::isSideSolo(track), juce::dontSendNotification);
    phaseBtn.setToggleState(MixerParameterBridge::isPhaseInverted(track), juce::dontSendNotification);
    
    // MS Mode Sync
    bool msMode = MixerParameterBridge::isMidSideMode(track);
    if (msToggle.getToggleState() != msMode) {
        msToggle.setToggleState(msMode, juce::dontSendNotification);
    }
    
    auto getDbVal = [](float gain) { return (gain <= 0.0001f) ? -64.0 : (double)juce::Decibels::gainToDecibels(gain, -100.0f); };
    
    fader.setValue(getDbVal(track->getVolume()), juce::dontSendNotification);
    midFader.setValue(getDbVal(MixerParameterBridge::getMidVolume(track)), juce::dontSendNotification);
    sideFader.setValue(getDbVal(MixerParameterBridge::getSideVolume(track)), juce::dontSendNotification);

    fader.setVisible(!msMode);
    meter.setVisible(!msMode);
    midFader.setVisible(msMode);
    sideFader.setVisible(msMode);
    midMeter.setVisible(msMode);
    sideMeter.setVisible(msMode);
    midLabel.setVisible(msMode);
    sideLabel.setVisible(msMode);

    if (trackName.getText() != track->getName()) 
        trackName.setText(track->getName(), juce::dontSendNotification);
        
    if (!isMiniMode) {
        fxRack.syncSlots();
        sendRack.syncSlots();
    }
    resized(); // Forzar re-layout si ha cambiado el modo
    repaint();
}

void MixerChannelUI::paint(juce::Graphics& g) {
    g.fillAll(getLookAndFeel().findColour(MixerColours::channelBackground));
    g.setColour(juce::Colours::black.withAlpha(0.3f)); 
    g.drawRect(getLocalBounds(), 1);
    g.setColour(track->getColor()); 
    g.fillRect(0, getHeight() - 5, getWidth(), 5);
}

void MixerChannelUI::resized() {
    auto b = getLocalBounds().reduced(4);

    if (!isMiniMode) {
        auto topArea = b.removeFromTop(100);
        auto panToggleArea = topArea.removeFromTop(20);
        panToggle.setBounds(panToggleArea.withSizeKeepingCentre(40, 18));

        if (MixerParameterBridge::isPanningModeDual(track)) {
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
    auto btnW = btnArea.getWidth() / 7;
    muteBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(1));
    soloBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(1));
    msToggle.setBounds(btnArea.removeFromLeft(btnW).reduced(1));
    midSoloBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(1));
    sideSoloBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(1));
    phaseBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(1));
    recBtn.setBounds(btnArea.removeFromLeft(btnW).reduced(1));

    trackName.setBounds(b.removeFromBottom(20));

    if (!MixerParameterBridge::isMidSideMode(track)) 
    {
        meter.setBounds(b.removeFromRight(isMiniMode ? 20 : 22));
        b.removeFromRight(3);
        fader.setBounds(b);
    }
    else 
    {
        auto msArea = b;
        auto midArea = msArea.removeFromLeft(msArea.getWidth() / 2);
        
        // Mid Side
        midLabel.setBounds(midArea.removeFromTop(12));
        midMeter.setBounds(midArea.removeFromRight(isMiniMode ? 14 : 16));
        midArea.removeFromRight(2);
        midFader.setBounds(midArea);

        // Side Area
        sideLabel.setBounds(msArea.removeFromTop(12));
        sideMeter.setBounds(msArea.removeFromRight(isMiniMode ? 14 : 16));
        msArea.removeFromRight(2);
        sideFader.setBounds(msArea);
    }
}

void MixerChannelUI::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isPopupMenu()) {
        if (e.originalComponent == &fader) {
            showAutomationMenu(0, "Volume");
        }
        else if (e.originalComponent == &panKnob || e.originalComponent == &panL || e.originalComponent == &panR) {
            showAutomationMenu(1, "Pan");
        }
    }
}

void MixerChannelUI::showAutomationMenu(int paramId, const juce::String& paramName) {
    juce::PopupMenu m;
    m.addItem(1, "Create Automation Clip");
    m.showMenuAsync(juce::PopupMenu::Options(), [this, paramId, paramName](int res) {
        if (res == 1 && onCreateAutomation) {
            onCreateAutomation(*track, paramId, paramName);
        }
    });
}

void MixerChannelUI::setupButton(juce::TextButton& btn, juce::String text, juce::Colour onCol) {
    btn.setButtonText(text); btn.setClickingTogglesState(true);
    btn.setColour(juce::TextButton::buttonOnColourId, onCol);
    btn.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    addAndMakeVisible(btn);
}

void MixerChannelUI::updatePanVisibility() {
    if (isMiniMode) return;
    bool dual = MixerParameterBridge::isPanningModeDual(track);
    panKnob.setVisible(!dual);
    panL.setVisible(dual);
    panR.setVisible(dual);
    resized();
}

void MixerChannelUI::timerCallback() {
    if (track != nullptr) {
        // --- ACTIVAR MOVIMIENTO FÍSICO SOLO SI HAY MODULACIÓN ACTIVA ---
        auto getDbVal = [](float gain) { return (gain <= 0.0001f) ? -64.0 : (double)juce::Decibels::gainToDecibels(gain, -100.0f); };

        if (!fader.isMouseButtonDown()) {
            double target = track->mixerData.visSync.hasVol.load(std::memory_order_relaxed) 
                ? getDbVal(track->mixerData.visSync.vol.load()) 
                : getDbVal(track->mixerData.volume);
            fader.setValue(NativeVisualSync::smooth((float)fader.getValue(), (float)target, 0.25f), juce::dontSendNotification);
        }

        if (!panKnob.isMouseButtonDown()) {
            float target = track->mixerData.visSync.hasPan.load(std::memory_order_relaxed)
                ? track->mixerData.visSync.hasPan.load() ? track->mixerData.visSync.pan.load() : track->mixerData.balance
                : track->mixerData.balance;
            panKnob.setValue(NativeVisualSync::smooth((float)panKnob.getValue(), target, 0.25f), juce::dontSendNotification);
        }

        meterLF.setTrackColour(track->getColor());
        meter.repaint();
        midMeter.repaint();
        sideMeter.repaint();
    }
}
