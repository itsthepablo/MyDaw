#include "TrackControlPanel.h"

TrackControlPanel::TrackControlPanel(Track& t) : track(t) 
{
    addAndMakeVisible(nameLabel);
    nameLabel.setText(track.getName(), juce::dontSendNotification);
    nameLabel.setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    
    track.addChangeListener(this);
    bool isAnalysisTrack = (track.getType() == TrackType::Loudness || 
                           track.getType() == TrackType::Balance || 
                           track.getType() == TrackType::MidSide);
    nameLabel.setEditable(!isAnalysisTrack);
    nameLabel.onTextChange = [this] { track.setName(nameLabel.getText()); };

    trackMeterLF.setTrackColour(track.getColor());
    levelMeter.setLookAndFeel(&trackMeterLF);

    muteBtn.setButtonText("M");
    muteBtn.setClickingTogglesState(true);
    muteBtn.setToggleState(MixerParameterBridge::isMuted(&track), juce::dontSendNotification);
    muteBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 52));
    muteBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::red.darker(0.2f));
    muteBtn.onClick = [this] { MixerParameterBridge::setMuted(&track, muteBtn.getToggleState()); };

    soloBtn.setButtonText("S");
    soloBtn.setClickingTogglesState(true);
    soloBtn.setToggleState(MixerParameterBridge::isSoloed(&track), juce::dontSendNotification);
    soloBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 52));
    soloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::yellow.darker(0.2f));
    soloBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    soloBtn.onClick = [this] { MixerParameterBridge::setSoloed(&track, soloBtn.getToggleState()); };

    volKnob.setName("TrackKnob");
    volKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    volKnob.setRange(0.0, 1.0);
    volKnob.setValue(track.getVolume(), juce::dontSendNotification);
    volKnob.onValueChange = [this] { track.setVolume((float)volKnob.getValue()); };

    volKnob.valueToTextFormattingCallback = [](double rawGain) -> juce::String {
        float db = juce::Decibels::gainToDecibels((float)rawGain, -100.0f);
        return db <= -100.0f ? "-inf dB" : juce::String(db, 1) + " dB";
    };

    panKnob.setName("TrackKnob");
    panKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panKnob.setRange(-1.0, 1.0);
    panKnob.setValue(track.getBalance(), juce::dontSendNotification);
    panKnob.onValueChange = [this] { track.setBalance((float)panKnob.getValue()); };

    panKnob.valueToTextFormattingCallback = [](double value) -> juce::String {
        if (std::abs(value) < 0.01) return "Center";
        int percent = (int)(std::abs(value) * 100.0);
        return juce::String(percent) + (value < 0.0 ? "% L" : "% R");
    };

    volKnob.addMouseListener(this, false);
    panKnob.addMouseListener(this, false);

    effectsBtn.setButtonText("Effects");
    effectsBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 65, 70));
    effectsBtn.onClick = [this] { if (onEffectsClick) onEffectsClick(); };

    addAndMakeVisible(folderBtn);
    addAndMakeVisible(compactBtn);

    folderBtn.onClick = [this] {
        if (track.getType() == TrackType::Folder) {
            track.isCollapsed = !track.isCollapsed;
            updateFolderBtnVisuals();
            if (onFolderStateChange) onFolderStateChange();
        }
    };

    if (track.getType() == TrackType::Loudness) {
        addAndMakeVisible(targetLufsCombo);
        targetLufsCombo.addItem("-23 LUFS (Broad)", 1);
        targetLufsCombo.addItem("-14 LUFS (Stream)", 2);
        targetLufsCombo.addItem("-5 LUFS (Loud)", 3);

        loudnessMeter = std::make_unique<LoudnessMeter>(track);
        addAndMakeVisible(*loudnessMeter);
        
        if (track.loudnessTrackData.history.referenceLUFS == -23.0f) targetLufsCombo.setSelectedId(1, juce::dontSendNotification);
        else if (track.loudnessTrackData.history.referenceLUFS == -14.0f) targetLufsCombo.setSelectedId(2, juce::dontSendNotification);
        else if (track.loudnessTrackData.history.referenceLUFS == -5.0f) targetLufsCombo.setSelectedId(3, juce::dontSendNotification);
        else targetLufsCombo.setText(juce::String((int)track.loudnessTrackData.history.referenceLUFS) + " LUFS", juce::dontSendNotification);

        targetLufsCombo.onChange = [this] {
            int id = targetLufsCombo.getSelectedId();
            if (id == 1) track.loudnessTrackData.history.referenceLUFS = -23.0f;
            else if (id == 2) track.loudnessTrackData.history.referenceLUFS = -14.0f;
            else if (id == 3) track.loudnessTrackData.history.referenceLUFS = -5.0f;
            repaint();
            if (onWaveformViewChanged) onWaveformViewChanged();
        };
    } else if (track.getType() == TrackType::Balance) {
        addAndMakeVisible(balanceScaleCombo);
        balanceScaleCombo.addItem("1x (12 dB)", 1);
        balanceScaleCombo.addItem("2x (6 dB)", 2);
        balanceScaleCombo.addItem("4x (3 dB)", 3);
        balanceScaleCombo.addItem("6x (2 dB)", 4);
        
        if (track.balanceTrackData.history.referenceScaleDB == 12.0f) balanceScaleCombo.setSelectedId(1, juce::dontSendNotification);
        else if (track.balanceTrackData.history.referenceScaleDB == 6.0f)  balanceScaleCombo.setSelectedId(2, juce::dontSendNotification);
        else if (track.balanceTrackData.history.referenceScaleDB == 3.0f)  balanceScaleCombo.setSelectedId(3, juce::dontSendNotification);
        else if (track.balanceTrackData.history.referenceScaleDB == 2.0f)  balanceScaleCombo.setSelectedId(4, juce::dontSendNotification);
        else {
            track.balanceTrackData.history.referenceScaleDB = 12.0f;
            balanceScaleCombo.setSelectedId(1, juce::dontSendNotification);
        }
        
        balanceScaleCombo.onChange = [this] {
            int id = balanceScaleCombo.getSelectedId();
            if      (id == 1) track.balanceTrackData.history.referenceScaleDB = 12.0f;
            else if (id == 2) track.balanceTrackData.history.referenceScaleDB = 6.0f;
            else if (id == 3) track.balanceTrackData.history.referenceScaleDB = 3.0f;
            else if (id == 4) track.balanceTrackData.history.referenceScaleDB = 2.0f;
            repaint();
            if (onWaveformViewChanged) onWaveformViewChanged();
        };
    } else if (track.getType() == TrackType::MidSide) {
        addAndMakeVisible(midSideScaleCombo);
        midSideScaleCombo.addItem("1x (Normal)", 1);
        midSideScaleCombo.addItem("2x (+6dB)", 2);
        midSideScaleCombo.addItem("4x (+12dB)", 3);
        midSideScaleCombo.addItem("8x (+18dB)", 4);
        
        if      (track.midSideTrackData.history.referenceScaleDB == 1.0f) midSideScaleCombo.setSelectedId(1, juce::dontSendNotification);
        else if (track.midSideTrackData.history.referenceScaleDB == 2.0f) midSideScaleCombo.setSelectedId(2, juce::dontSendNotification);
        else if (track.midSideTrackData.history.referenceScaleDB == 4.0f) midSideScaleCombo.setSelectedId(3, juce::dontSendNotification);
        else if (track.midSideTrackData.history.referenceScaleDB == 8.0f) midSideScaleCombo.setSelectedId(4, juce::dontSendNotification);
        else midSideScaleCombo.setSelectedId(1, juce::dontSendNotification);

        midSideScaleCombo.onChange = [this] {
            int id = midSideScaleCombo.getSelectedId();
            if      (id == 1) track.midSideTrackData.history.referenceScaleDB = 1.0f;
            else if (id == 2) track.midSideTrackData.history.referenceScaleDB = 2.0f;
            else if (id == 3) track.midSideTrackData.history.referenceScaleDB = 4.0f;
            else if (id == 4) track.midSideTrackData.history.referenceScaleDB = 8.0f;
            repaint();
            if (onWaveformViewChanged) onWaveformViewChanged();
        };

        addAndMakeVisible(midSideModeCombo);
        midSideModeCombo.addItem("Overlay (Both)", 1);
        midSideModeCombo.addItem("Mid Only", 2);
        midSideModeCombo.addItem("Side Only", 3);
        midSideModeCombo.setSelectedId(track.midSideTrackData.history.displayMode + 1, juce::dontSendNotification);
        midSideModeCombo.onChange = [this] {
            track.midSideTrackData.history.displayMode = midSideModeCombo.getSelectedId() - 1;
            repaint();
            if (onWaveformViewChanged) onWaveformViewChanged();
        };
    } else {
        addAndMakeVisible(muteBtn);
        addAndMakeVisible(soloBtn);
        addAndMakeVisible(midSoloBtn);
        addAndMakeVisible(sideSoloBtn);
        addAndMakeVisible(volKnob);
        addAndMakeVisible(panKnob);
        addAndMakeVisible(levelMeter);
        addAndMakeVisible(effectsBtn);
    }

    if (track.getType() == TrackType::MIDI) {
        addAndMakeVisible(prButton);
        prButton.setButtonText("Piano Roll");
        
        addAndMakeVisible(inlineBtn);
        inlineBtn.setButtonText("Inline");
        inlineBtn.setClickingTogglesState(true);
        inlineBtn.setToggleState(track.isInlineEditingActive, juce::dontSendNotification);
        
        addAndMakeVisible(fxButton);
        fxButton.setButtonText("VSTi");
        
        prButton.onClick = [this] { if (onPianoRollClick) onPianoRollClick(); };
        inlineBtn.onClick = [this] {
            track.isInlineEditingActive = inlineBtn.getToggleState();
            if (onWaveformViewChanged) onWaveformViewChanged();
        };
        fxButton.onClick = [this] { if (onInstrumentClick) onInstrumentClick(); };
    }

    midSoloBtn.setButtonText("MID");
    midSoloBtn.setTooltip("Solo MID (Centro)");
    midSoloBtn.setClickingTogglesState(true);
    midSoloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::blue.withAlpha(0.6f));
    midSoloBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    midSoloBtn.onClick = [this] { 
        MixerParameterBridge::setMidSolo(&track, midSoloBtn.getToggleState()); 
    };

    sideSoloBtn.setButtonText("SID");
    sideSoloBtn.setTooltip("Solo SIDE (Laterales)");
    sideSoloBtn.setClickingTogglesState(true);
    sideSoloBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::purple.withAlpha(0.6f));
    sideSoloBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    sideSoloBtn.onClick = [this] { 
        MixerParameterBridge::setSideSolo(&track, sideSoloBtn.getToggleState());
    };

    updateFolderBtnVisuals();
    startTimerHz(30);
}

TrackControlPanel::~TrackControlPanel() 
{ 
    track.removeChangeListener(this);
    levelMeter.setLookAndFeel(nullptr);
    stopTimer(); 
}

void TrackControlPanel::timerCallback() 
{
    if (track.getType() == TrackType::Loudness || track.getType() == TrackType::Balance || track.getType() == TrackType::MidSide) return; 
    trackMeterLF.setTrackColour(track.getColor());
    
    muteBtn.setToggleState(MixerParameterBridge::isMuted(&track), juce::dontSendNotification);
    soloBtn.setToggleState(MixerParameterBridge::isSoloed(&track), juce::dontSendNotification);
    midSoloBtn.setToggleState(MixerParameterBridge::isMidSolo(&track), juce::dontSendNotification);
    sideSoloBtn.setToggleState(MixerParameterBridge::isSideSolo(&track), juce::dontSendNotification);

    if (MixerParameterBridge::isMuted(&track)) { levelMeter.setLevel(0.0f, 0.0f); return; }
    float vol = MixerParameterBridge::getVolume(&track);
    float pan = MixerParameterBridge::getBalance(&track);
    float leftGain = vol * (pan < 0.0f ? 1.0f : 1.0f - pan);
    float rightGain = vol * (pan > 0.0f ? 1.0f : 1.0f + pan);
    levelMeter.setLevel(MixerParameterBridge::getPeakLevelL(&track) * leftGain, MixerParameterBridge::getPeakLevelR(&track) * rightGain);
}

void TrackControlPanel::updateFolderBtnVisuals() 
{
    if (track.getType() == TrackType::Folder) {
        folderBtn.setButtonText(track.isCollapsed ? "+" : "-");
        folderBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(80, 85, 95));
        folderBtn.setVisible(true);
    } else { folderBtn.setVisible(false); }
    compactBtn.setVisible(false);
}

void TrackControlPanel::updatePlugins() 
{
    pluginButtons.clear();
    for (int i = 0; i < track.plugins.size(); ++i) {
        juce::String name = "VST3";
        if (track.plugins[i] != nullptr && track.plugins[i]->isLoaded()) name = track.plugins[i]->getLoadedPluginName();
        auto* btn = new juce::TextButton(name);
        btn->onClick = [this, i] { if (onPluginClick) onPluginClick(i); };
        pluginButtons.add(btn); addAndMakeVisible(btn);
    }
    resized();
}

void TrackControlPanel::paint(juce::Graphics& g) 
{
    g.fillAll(juce::Colour(30, 32, 35));
    auto area = getLocalBounds();
    int indent = track.folderDepth * 15;
    
    auto colorBarArea = area.withTrimmedLeft(indent + 4).removeFromLeft(6).reduced(0, 4);
    g.setColour(track.getColor());
    g.fillRoundedRectangle(colorBarArea.toFloat(), 3.0f);

    auto contentArea = area.withTrimmedLeft(indent + 4 + 6 + 4).withTrimmedRight(20);
    g.setColour(track.getColor().withAlpha(0.15f));
    g.fillRoundedRectangle(contentArea.toFloat().reduced(0, 2), 4.0f);

    if (track.folderDepth > 0) {
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        for (int i = 1; i <= track.folderDepth; ++i) {
            float lineX = (float)((i * 15) - 7.5f);
            g.drawVerticalLine((int)lineX, 2.0f, (float)getHeight() - 2.0f);
            if (i == track.folderDepth) g.drawHorizontalLine(getHeight() / 2, lineX, (float)indent);
        }
    }

    if (track.getType() == TrackType::Folder) {
        auto iconArea = contentArea.removeFromTop(20).removeFromLeft(20).reduced(5);
        g.setColour(juce::Colours::orange.withAlpha(0.6f));
        g.fillRect(iconArea);
    }

    if (track.isSelected) {
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.fillRoundedRectangle(contentArea.toFloat().reduced(0, 2), 4.0f);
        
        // Borde de selección más visible
        g.setColour(juce::Colours::white);
        g.drawRoundedRectangle(contentArea.toFloat().reduced(0, 2), 4.0f, 2.0f);
    }

    if (dragHoverMode != 0) {
        g.setColour(dragHoverMode == 2 ? juce::Colours::cyan : juce::Colours::white);
        if (dragHoverMode == 1) g.fillRect(0, 0, getWidth(), 2);
        else if (dragHoverMode == 3) g.fillRect(indent, getHeight() - 2, getWidth() - indent, 2);
        else if (dragHoverMode == 2) g.drawRect(contentArea, 2);
    }
}

void TrackControlPanel::resized() 
{
    int indent = track.folderDepth * 15;
    auto area = getLocalBounds();
    auto contentArea = area.withTrimmedLeft(indent + 4 + 6 + 4).withTrimmedRight(20).reduced(4, 2);

    auto leftCol = contentArea.removeFromLeft(125);
    auto topRow = leftCol.removeFromTop(20);
    if (track.getType() == TrackType::Folder) folderBtn.setBounds(topRow.removeFromLeft(20).reduced(2));
    nameLabel.setBounds(topRow);
    
    leftCol.removeFromTop(2);
    
    if (track.getType() == TrackType::Loudness) {
        auto topArea = leftCol.removeFromTop(20);
        targetLufsCombo.setBounds(topArea.removeFromRight(85).reduced(2));
        
        if (loudnessMeter)
            loudnessMeter->setBounds(leftCol.withTrimmedTop(4).reduced(2));
    } else if (track.getType() == TrackType::Balance) {
        balanceScaleCombo.setBounds(leftCol.removeFromTop(22).reduced(2));
    } else if (track.getType() == TrackType::MidSide) {
        midSideScaleCombo.setBounds(leftCol.removeFromTop(22).reduced(2));
        midSideModeCombo.setBounds(leftCol.removeFromTop(22).reduced(2));
    } else {
        auto kRow = leftCol.removeFromTop(28);
        muteBtn.setBounds(kRow.removeFromLeft(20).reduced(1));
        soloBtn.setBounds(kRow.removeFromLeft(20).reduced(1));
        midSoloBtn.setBounds(kRow.removeFromLeft(20).reduced(1));
        sideSoloBtn.setBounds(kRow.removeFromLeft(20).reduced(1));
        panKnob.setBounds(kRow.removeFromLeft(22));
        volKnob.setBounds(kRow.removeFromLeft(22));
        
        auto botRow = leftCol.removeFromTop(18);
        effectsBtn.setBounds(botRow.reduced(2, 0));
    }

    if (track.getType() == TrackType::MIDI) {
        auto prRow = leftCol.removeFromTop(18);
        prButton.setBounds(prRow.removeFromLeft(prRow.getWidth() / 2).reduced(1));
        inlineBtn.setBounds(prRow.reduced(1));
    }

    if (track.getType() != TrackType::Loudness && track.getType() != TrackType::Balance && track.getType() != TrackType::MidSide) {
        trackMeterLF.setTrackColour(track.getColor());
        auto meterArea = area.removeFromRight(20).withTrimmedLeft(4).withTrimmedRight(4).reduced(0, 2);
        levelMeter.setBounds(meterArea);
    }

    auto fxArea = contentArea.removeFromRight(110);
    if (track.getType() == TrackType::MIDI) {
        fxButton.setBounds(fxArea.removeFromTop(18).reduced(1));
        fxArea.removeFromTop(2);
    }
    for (auto* b : pluginButtons) {
        b->setBounds(fxArea.removeFromTop(18).reduced(1));
        fxArea.removeFromTop(2);
    }
}

void TrackControlPanel::mouseDown(const juce::MouseEvent& e) 
{
    grabKeyboardFocus();
    if (e.mods.isPopupMenu()) {
        if (e.originalComponent == &volKnob) {
            showAutomationMenu(0, "Volume");
            return;
        } else if (e.originalComponent == &panKnob) {
            showAutomationMenu(1, "Pan");
            return;
        }
    }

    if (e.mods.isLeftButtonDown() && onTrackSelected) onTrackSelected(e.mods);
    if (e.mods.isRightButtonDown()) {
        juce::PopupMenu m;
        m.addItem(1, "Eliminar Pista");
        m.addItem(7, "Move to New Routing Folder", onMoveToNewFolder != nullptr);
        m.addSeparator();
        m.addItem(6, "Cambiar Color...");
        if (track.getType() == TrackType::Audio) {
            m.addSeparator();
            m.addItem(2, "Vista: Combinada (L+R y Polaridad)", true, track.getWaveformViewMode() == WaveformViewMode::Combined);
            m.addItem(3, "Vista: Separada (L / R)", true, track.getWaveformViewMode() == WaveformViewMode::SeparateLR);
            m.addItem(4, "Vista: Mid / Side", true, track.getWaveformViewMode() == WaveformViewMode::MidSide);
            m.addItem(5, "Vista: Espectrograma (Frecuencias)", true, track.getWaveformViewMode() == WaveformViewMode::Spectrogram);
        }
        if (track.getType() == TrackType::Loudness) {
            m.addSeparator();
            m.addItem(10, "Borrar Historial de Sonoridad");
        }
        m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) {
            if (r == 1 && onDeleteClick) onDeleteClick();
            else if (r == 7 && onMoveToNewFolder) onMoveToNewFolder();
            else if (r == 6) {
                auto* cs = new juce::ColourSelector(juce::ColourSelector::showColourAtTop | juce::ColourSelector::showSliders | juce::ColourSelector::showColourspace);
                cs->setCurrentColour(track.getColor()); cs->addChangeListener(this); cs->setSize(300, 400);
                juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(cs), getScreenBounds(), nullptr);
            } else if (r >= 2 && r <= 5 && onWaveformViewChanged) { track.setWaveformViewMode((WaveformViewMode)(r-2)); onWaveformViewChanged(); }
            else if (r == 10) { track.loudnessTrackData.history.clear(); if (onWaveformViewChanged) onWaveformViewChanged(); }
        });
    }
}

void TrackControlPanel::changeListenerCallback(juce::ChangeBroadcaster* s) 
{
    if (s == &track)
    {
        nameLabel.setText(track.getName(), juce::dontSendNotification);
        trackMeterLF.setTrackColour(track.getColor());
        repaint();
    }
    else if (auto* cs = dynamic_cast<juce::ColourSelector*>(s)) 
    {
        track.setColor(cs->getCurrentColour(), true); // Asignación manual
        repaint(); 
        if (onTrackColorChanged) onTrackColorChanged();
    }
}

void TrackControlPanel::mouseDrag(const juce::MouseEvent& e) 
{
    if (e.originalComponent == &volKnob || e.originalComponent == &panKnob || 
        e.originalComponent == &muteBtn || e.originalComponent == &soloBtn ||
        e.originalComponent == &fxButton || e.originalComponent == &prButton ||
        e.originalComponent == &inlineBtn || e.originalComponent == &effectsBtn ||
        e.originalComponent == &targetLufsCombo || e.originalComponent == &balanceScaleCombo)
        return;

    if (auto* dc = juce::DragAndDropContainer::findParentDragContainerFor(this)) {
        juce::Image g(juce::Image::ARGB, 1, 1, true); dc->startDragging("TRACK", this, g, true);
    }
}

bool TrackControlPanel::keyPressed(const juce::KeyPress& k) 
{
    if (k.getKeyCode() == juce::KeyPress::deleteKey || k.getKeyCode() == juce::KeyPress::backspaceKey) { if (onDeleteClick) onDeleteClick(); return true; }
    return false;
}

void TrackControlPanel::showAutomationMenu(int paramId, const juce::String& paramName) 
{
    juce::PopupMenu m;
    m.addItem(1, "Create Automation Clip");
    m.showMenuAsync(juce::PopupMenu::Options(), [this, paramId, paramName](int res) {
        if (res == 1 && onCreateAutomation) {
            onCreateAutomation(paramId, paramName);
        }
    });
}
