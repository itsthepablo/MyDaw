#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "HexColorPicker.h" 
#include "LissajousBackground.h" 
#include <cmath> 

//==============================================================================
MiPrimerVSTAudioProcessorEditor::MiPrimerVSTAudioProcessorEditor(MiPrimerVSTAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    int w = (audioProcessor.windowWidth < 100) ? 181 : audioProcessor.windowWidth;
    int h = (int)((double)w * (703.0 / 181.0));
    setSize(w, h);

    setResizable(true, false);
    getConstrainer()->setFixedAspectRatio(181.0 / 703.0);
    setResizeLimits(120, 468, 482, 1874);

    resizer = std::make_unique<juce::ResizableCornerComponent>(this, getConstrainer());
    resizer->setAlpha(0.0f);
    addAndMakeVisible(resizer.get());

    scopeColor = audioProcessor.savedScopeColor;
    meterLow = audioProcessor.savedMeterLow;
    meterMid = audioProcessor.savedMeterMid;
    meterHigh = audioProcessor.savedMeterHigh;
    correlationPosColor = audioProcessor.savedCorrelationPosColor;

    vuButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    vuButton.setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
    vuButton.onClick = [this] {
        centralViewMode = (centralViewMode == 1) ? 0 : 1;
        updateTopButtons();
        };
    addAndMakeVisible(vuButton);

    balanceButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    balanceButton.setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
    balanceButton.onClick = [this] {
        centralViewMode = (centralViewMode == 2) ? 0 : 2;
        updateTopButtons();
        };
    addAndMakeVisible(balanceButton);

    fftModeButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    fftModeButton.setColour(juce::TextButton::textColourOffId, juce::Colours::grey);
    fftModeButton.onClick = [this] {
        bool isMaster = !audioProcessor.btnFftMaster->get();
        *audioProcessor.btnFftMaster = isMaster;
        updateTopButtons();
        };
    addAndMakeVisible(fftModeButton);

    vuCalibKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    vuCalibKnob.setLookAndFeel(&customKnob);
    vuCalibKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    vuCalibKnob.setRange(0.0, 24.0, 0.1);
    vuCalibKnob.setValue(audioProcessor.vuCalibParam->get(), juce::dontSendNotification);
    vuCalibKnob.addListener(this);
    addAndMakeVisible(vuCalibKnob);
    vuCalibKnob.setVisible(false);

    menuButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    menuButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);

    menuButton.onClick = [this] {
        juce::PopupMenu m;
        m.addSectionHeader("COLORS");
        m.addItem(1, "Scope Color");
        m.addItem(2, "Meter Low");
        m.addItem(3, "Meter Mid");
        m.addItem(4, "Meter High");
        m.addSeparator();
        m.addItem(5, "Correlation (+)");

        m.showMenuAsync(juce::PopupMenu::Options().withParentComponent(this).withTargetComponent(menuButton),
            [this](int result) {
                if (result == 0) return;
                juce::Colour* target = nullptr; juce::Colour* save = nullptr;
                switch (result) {
                case 1: target = &scopeColor; save = &audioProcessor.savedScopeColor; break;
                case 2: target = &meterLow;   save = &audioProcessor.savedMeterLow;   break;
                case 3: target = &meterMid;   save = &audioProcessor.savedMeterMid;   break;
                case 4: target = &meterHigh;  save = &audioProcessor.savedMeterHigh;  break;
                case 5: target = &correlationPosColor; save = &audioProcessor.savedCorrelationPosColor; break;
                }
                if (target) {
                    auto* picker = new HexColorPicker(*target, [this, target, save](juce::Colour c) {
                        *target = c; *save = c; repaint();
                        });
                    juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(picker), menuButton.getScreenBounds(), nullptr);
                }
            });
        };
    addAndMakeVisible(menuButton);

    modeKnob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    modeKnob.setLookAndFeel(&customKnob);
    modeKnob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    modeKnob.setRange(0.0, 7.0, 1.0);
    modeKnob.setValue((double)audioProcessor.modeSelector->getIndex(), juce::dontSendNotification);
    modeKnob.addListener(this);
    addAndMakeVisible(modeKnob);

    modeDisplay.setJustificationType(juce::Justification::centred);
    modeDisplay.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    modeDisplay.setColour(juce::Label::textColourId, juce::Colours::cyan);
    modeDisplay.setInterceptsMouseClicks(false, false);
    updateModeText();
    addAndMakeVisible(modeDisplay);

    auto configButton = [this](juce::TextButton& btn, int id) {
        btn.setClickingTogglesState(true);
        addAndMakeVisible(btn);
        btn.onClick = [this, &btn, id] {
            bool isNowOn = btn.getToggleState();

            if (isNowOn && id <= 3) {
                if (id == 1) { *audioProcessor.btnMid = false; midButton.setToggleState(false, juce::dontSendNotification); *audioProcessor.btnHigh = false; highButton.setToggleState(false, juce::dontSendNotification); }
                if (id == 2) { *audioProcessor.btnLow = false; lowButton.setToggleState(false, juce::dontSendNotification); *audioProcessor.btnHigh = false; highButton.setToggleState(false, juce::dontSendNotification); }
                if (id == 3) { *audioProcessor.btnLow = false; lowButton.setToggleState(false, juce::dontSendNotification); *audioProcessor.btnMid = false; midButton.setToggleState(false, juce::dontSendNotification); }
            }

            if (id == 1) *audioProcessor.btnLow = isNowOn;
            if (id == 2) *audioProcessor.btnMid = isNowOn;
            if (id == 3) *audioProcessor.btnHigh = isNowOn;
            if (id == 4) *audioProcessor.btnPhase = isNowOn;
            if (id == 5) {
                *audioProcessor.btnBypass = isNowOn;
                updateModeText();
            }

            updateButtonColors();
            };
        };

    configButton(lowButton, 1);
    configButton(midButton, 2);
    configButton(highButton, 3);
    configButton(phaseButton, 4);
    configButton(bypassButton, 5);

    lowButton.setToggleState(*audioProcessor.btnLow, juce::dontSendNotification);
    midButton.setToggleState(*audioProcessor.btnMid, juce::dontSendNotification);
    highButton.setToggleState(*audioProcessor.btnHigh, juce::dontSendNotification);
    phaseButton.setToggleState(*audioProcessor.btnPhase, juce::dontSendNotification);
    bypassButton.setToggleState(*audioProcessor.btnBypass, juce::dontSendNotification);

    updateButtonColors();
    updateTopButtons();

    startTimerHz(50);
}

MiPrimerVSTAudioProcessorEditor::~MiPrimerVSTAudioProcessorEditor() {
    stopTimer();
    setVisible(false);
    modeKnob.setLookAndFeel(nullptr);
}

void MiPrimerVSTAudioProcessorEditor::updateTopButtons() {
    vuButton.setColour(juce::TextButton::textColourOffId, (centralViewMode == 1) ? juce::Colours::cyan : juce::Colours::grey);
    balanceButton.setColour(juce::TextButton::textColourOffId, (centralViewMode == 2) ? juce::Colours::cyan : juce::Colours::grey);

    modeKnob.setVisible(centralViewMode == 0);
    modeDisplay.setVisible(centralViewMode == 0);
    vuCalibKnob.setVisible(centralViewMode == 1);

    bool isMaster = audioProcessor.btnFftMaster->get();
    fftModeButton.setVisible(centralViewMode == 2);
    fftModeButton.setColour(juce::TextButton::textColourOffId, isMaster ? juce::Colours::orange : juce::Colours::cyan);
    fftModeButton.setButtonText(isMaster ? "M" : "D");

    repaint();
}

void MiPrimerVSTAudioProcessorEditor::timerCallback() {
    float peakDecay = 0.15f;
    auto updatePeakText = [peakDecay](float& displayedVal, float currentRealVal) {
        if (currentRealVal > displayedVal) displayedVal = currentRealVal;
        else displayedVal += (currentRealVal - displayedVal) * peakDecay;
        };
    updatePeakText(txtPeakL, audioProcessor.peakLevelLeft.load());
    updatePeakText(txtPeakR, audioProcessor.peakLevelRight.load());

    const int holdFrames = 120;
    auto updateRmsHold = [&](float& displayedVal, float currentVal, int& counter) {
        if (currentVal > displayedVal) { displayedVal = currentVal; counter = holdFrames; }
        else { if (counter > 0) counter--; else displayedVal = currentVal; }
        };
    updateRmsHold(txtRmsL, audioProcessor.rmsLevelLeft.load(), rmsHoldCounterL);
    updateRmsHold(txtRmsR, audioProcessor.rmsLevelRight.load(), rmsHoldCounterR);

    waveformEngine.pushSamples(audioProcessor.scopeBufferL, audioProcessor.scopeBufferR, MiPrimerVSTAudioProcessor::scopeSize);
    legacyScope.pushBuffer(audioProcessor.scopeBufferL, audioProcessor.scopeBufferR, MiPrimerVSTAudioProcessor::scopeSize);
    proLScopeEngine.pushSamples(audioProcessor.scopeBufferL, audioProcessor.scopeBufferR, MiPrimerVSTAudioProcessor::scopeSize);

    if (audioProcessor.savedVectorscopeMode == 1) {
        polarEngine.process(audioProcessor.scopeBufferL, audioProcessor.scopeBufferR, MiPrimerVSTAudioProcessor::scopeSize);
    }

    uiVuL = audioProcessor.vuLevelL.load();
    uiVuR = audioProcessor.vuLevelR.load();
    uiVuPeakL = audioProcessor.vuPeakL.load();
    uiVuPeakR = audioProcessor.vuPeakR.load();
    uiVuCalib = audioProcessor.vuCalibParam->get();

    if (centralViewMode == 2) {
        audioProcessor.spectrumAnalyzer.process();
    }

    repaint();

    auto mousePos = getMouseXYRelative();
    auto bottomRight = getLocalBounds().getBottomRight();
    float distance = (float)mousePos.getDistanceFrom(bottomRight);
    float targetAlpha = (distance < 100.0f) ? 1.0f : 0.0f;
    float currentAlpha = resizer->getAlpha();
    if (std::abs(targetAlpha - currentAlpha) > 0.01f) resizer->setAlpha(currentAlpha + (targetAlpha - currentAlpha) * 0.15f);
    else resizer->setAlpha(targetAlpha);
}

void MiPrimerVSTAudioProcessorEditor::updateModeText() {
    if (audioProcessor.btnBypass->get()) {
        modeDisplay.setText("BYPASS", juce::dontSendNotification);
        modeDisplay.setColour(juce::Label::textColourId, juce::Colours::orange);
    }
    else {
        int idx = audioProcessor.modeSelector->getIndex();
        if (idx >= 0 && idx < audioProcessor.modeOptions.size())
            modeDisplay.setText(audioProcessor.modeOptions[idx], juce::dontSendNotification);
        modeDisplay.setColour(juce::Label::textColourId, juce::Colours::cyan);
    }
}

void MiPrimerVSTAudioProcessorEditor::updateButtonColors() {
    auto setCol = [](juce::TextButton& btn, juce::Colour onColor) {
        if (btn.getToggleState()) btn.setColour(juce::TextButton::buttonColourId, onColor);
        else btn.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
        };

    setCol(lowButton, juce::Colour::fromRGB(100, 100, 255));
    setCol(midButton, juce::Colour::fromRGB(200, 50, 200));
    setCol(highButton, juce::Colour::fromRGB(255, 100, 200));
    setCol(phaseButton, juce::Colour::fromRGB(255, 140, 0));
    setCol(bypassButton, juce::Colours::orange);
}

void MiPrimerVSTAudioProcessorEditor::resized()
{
    if (isVisible()) {
        audioProcessor.windowWidth = getWidth();
        audioProcessor.windowHeight = getHeight();
    }
    scaleFactor = (float)getWidth() / 181.0f;
    juce::AffineTransform transform = juce::AffineTransform::scale(scaleFactor);
    juce::Rectangle<int> virtualCanvas(0, 0, 181, 703);
    auto vHeader = virtualCanvas.removeFromTop(25); rectHeaderBg = vHeader;

    vuButton.setBounds(6, 4, 25, 18);
    vuButton.setTransform(transform);

    balanceButton.setBounds(34, 4, 20, 18);
    balanceButton.setTransform(transform);

    fftModeButton.setBounds(57, 4, 20, 18);
    fftModeButton.setTransform(transform);

    menuButton.setBounds(154, 2, 24, 24);
    menuButton.setTransform(transform);

    auto vKnobArea = virtualCanvas.removeFromTop(160);
    int knobSize = 140; int knobX = (181 - knobSize) / 2; int knobY = vKnobArea.getY() + 5;

    juce::Rectangle<int> vKnobRect(knobX, knobY, knobSize, knobSize);
    juce::Rectangle<int> vModeRect(vKnobRect.getX(), vKnobRect.getBottom() - 8, vKnobRect.getWidth(), 20);
    auto vStatsArea = virtualCanvas.removeFromTop(35);

    auto vBtnsArea = virtualCanvas.removeFromTop(30);
    int btnW = 26; int btnH = 18; int gap = 3;
    int totalBtnsWidth = (btnW * 5) + (gap * 4);
    int startX = (181 - totalBtnsWidth) / 2;
    int btnY = vBtnsArea.getY() + 6;

    int calibSize = 34;
    juce::Rectangle<int> vCalibRect((181 - calibSize) / 2, vStatsArea.getBottom() - calibSize - 5, calibSize, calibSize);
    vuCalibKnob.setBounds(vCalibRect);
    vuCalibKnob.setTransform(transform);

    juce::Rectangle<int> vLow(startX, btnY, btnW, btnH);
    juce::Rectangle<int> vMid(startX + btnW + gap, btnY, btnW, btnH);
    juce::Rectangle<int> vHigh(startX + (btnW + gap) * 2, btnY, btnW, btnH);
    juce::Rectangle<int> vPhase(startX + (btnW + gap) * 3, btnY, btnW, btnH);
    juce::Rectangle<int> vBypass(startX + (btnW + gap) * 4, btnY, btnW, btnH);

    auto vMeters = virtualCanvas.removeFromTop(215).reduced(4, 0);

    juce::Rectangle<int> vPeak = vMeters.removeFromLeft(vMeters.getWidth() / 2).reduced(2, 0);
    juce::Rectangle<int> vRms = vMeters.reduced(2, 0);

    auto vScopes = virtualCanvas.reduced(8, 8);
    juce::Rectangle<int> vOsc = vScopes.removeFromTop((int)(vScopes.getHeight() * 0.40f)).reduced(0, 8);
    vScopes.removeFromTop(8);
    juce::Rectangle<int> vVec = vScopes.reduced(0, 4);

    modeKnob.setBounds(vKnobRect); modeKnob.setTransform(transform);
    modeDisplay.setBounds(vModeRect); modeDisplay.setTransform(transform);

    lowButton.setBounds(vLow); lowButton.setTransform(transform);
    midButton.setBounds(vMid); midButton.setTransform(transform);
    highButton.setBounds(vHigh); highButton.setTransform(transform);
    phaseButton.setBounds(vPhase); phaseButton.setTransform(transform);
    bypassButton.setBounds(vBypass); bypassButton.setTransform(transform);

    rectKnob = vKnobRect; rectStats = vStatsArea; rectPeak = vPeak; rectRms = vRms;
    rectOscilloscope = vOsc; rectVectorscope = vVec;
    if (resizer) resizer->setBounds(getWidth() - 20, getHeight() - 20, 20, 20);

    vuButton.toFront(true);
    balanceButton.toFront(true);
    fftModeButton.toFront(true);
    menuButton.toFront(true);
}

void MiPrimerVSTAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(18, 18, 20));

    g.addTransform(juce::AffineTransform::scale(scaleFactor));
    if (menuButton.isMouseOver()) g.setColour(juce::Colours::white);
    else g.setColour(juce::Colours::white.withAlpha(0.4f));
    auto b = menuButton.getBounds();
    int mx = b.getX() + 4; int my = b.getY() + 6; int mw = 16;
    g.fillRect(mx, my, mw, 2); g.fillRect(mx, my + 5, mw, 2); g.fillRect(mx, my + 10, mw, 2);

    juce::Rectangle<int> centralBounds = rectKnob.getUnion(rectStats);

    if (centralViewMode == 1) {
        VuMeterGUI::draw(g, centralBounds, uiVuL, uiVuR, uiVuPeakL, uiVuPeakR, uiVuCalib, vuViewMode);
        g.setFont(8.5f);
        g.setColour(juce::Colours::grey.withAlpha(0.6f));
        g.drawText("CALIB", vuCalibKnob.getBounds().withY(vuCalibKnob.getY() - 10).withHeight(10), juce::Justification::centred);
    }
    else if (centralViewMode == 2) {
        drawFFT(g, centralBounds);
    }
    else {
        drawLufsCentral(g, rectKnob);
        drawLufsStats(g, rectStats);
    }

    int labelH = 15; int titlePadding = 6; int clipH = 6; int textValH = 20; int gap = 2;
    int barW = 30; int gapBars = 4;

    auto drawPair = [&](juce::Rectangle<int> area, float valL, float valR, float txtL, float txtR, juce::String title, bool isRMS) {
        int meterTopY = area.getY() + labelH + titlePadding;
        int meterBottomY = area.getBottom() - textValH - gap;
        int totalBarH = meterBottomY - (meterTopY + clipH + gap);
        int barY = meterTopY + clipH + gap;
        int centerX = area.getX() + area.getWidth() / 2;
        int xL = centerX - (gapBars / 2) - barW;
        int xR = centerX + (gapBars / 2);

        const juce::Colour offLedColor = juce::Colours::grey.darker(0.7f);

        if (!isRMS) {
            g.setColour(valL >= 1.0f ? juce::Colours::red : offLedColor);
            g.fillRect(xL, meterTopY, barW, clipH);
        }
        else {
            float dbL = (valL > 0.0f) ? juce::Decibels::gainToDecibels(valL) : -100.0f;
            g.setColour(dbL > -5.0f ? juce::Colours::cyan : offLedColor);
            g.fillRect(xL, meterTopY, barW, clipH);
        }
        drawMeter(g, xL, barY, barW, totalBarH, valL, isRMS);

        if (!isRMS) {
            g.setColour(valR >= 1.0f ? juce::Colours::red : offLedColor);
            g.fillRect(xR, meterTopY, barW, clipH);
        }
        else {
            float dbR = (valR > 0.0f) ? juce::Decibels::gainToDecibels(valR) : -100.0f;
            g.setColour(dbR > -5.0f ? juce::Colours::cyan : offLedColor);
            g.fillRect(xR, meterTopY, barW, clipH);
        }
        drawMeter(g, xR, barY, barW, totalBarH, valR, isRMS);

        int txtWidth = 58;
        int centerBarL = xL + (barW / 2);
        int centerBarR = xR + (barW / 2);
        juce::Rectangle<int> txtRectL(centerBarL - (txtWidth / 2), area.getBottom() - textValH, txtWidth, textValH);
        juce::Rectangle<int> txtRectR(centerBarR - (txtWidth / 2), area.getBottom() - textValH, txtWidth, textValH);

        g.setColour(juce::Colours::white);
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(getDbString(txtL), txtRectL, juce::Justification::centred);
        g.drawText(getDbString(txtR), txtRectR, juce::Justification::centred);
        };

    drawPair(rectPeak, audioProcessor.peakLevelLeft, audioProcessor.peakLevelRight, txtPeakL, txtPeakR, "PEAK", false);
    drawPair(rectRms, audioProcessor.rmsLevelLeft, audioProcessor.rmsLevelRight, txtRmsL, txtRmsR, "RMS", true);

    drawOscilloscope(g, rectOscilloscope.getX(), rectOscilloscope.getY(), rectOscilloscope.getWidth(), rectOscilloscope.getHeight());
    drawVectorscope(g, rectVectorscope.getX(), rectVectorscope.getY(), rectVectorscope.getWidth(), rectVectorscope.getHeight());
}

// --- DIBUJO DEL ANALIZADOR DE ESPECTRO CON CROSSHAIR (EJE X Y) ---
void MiPrimerVSTAudioProcessorEditor::drawFFT(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    bounds = bounds.reduced(4);

    g.setColour(juce::Colour::fromRGB(16, 16, 18));
    g.fillRoundedRectangle(bounds.toFloat(), 6.0f);
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(bounds.toFloat().reduced(0.5f), 6.0f, 1.0f);

    const auto& specData = audioProcessor.spectrumAnalyzer.getSpectrumData();
    if (specData.empty()) return;

    juce::Path p;
    float bottomY = (float)bounds.getBottom();
    float w = (float)bounds.getWidth();

    p.startNewSubPath((float)bounds.getX(), bottomY);

    for (int i = 0; i < (int)specData.size(); ++i)
    {
        float x = (float)bounds.getX() + (i / (float)(specData.size() - 1)) * w;
        float db = specData[i];

        float y = juce::jmap(db, -90.0f, 0.0f, bottomY, (float)bounds.getY());
        y = juce::jlimit((float)bounds.getY(), bottomY, y);

        p.lineTo(x, y);
    }

    p.lineTo((float)bounds.getRight(), bottomY);
    p.closeSubPath();

    juce::ColourGradient grad(scopeColor.withAlpha(0.5f), (float)bounds.getCentreX(), (float)bounds.getY(),
        scopeColor.withAlpha(0.0f), (float)bounds.getCentreX(), bottomY, false);
    g.setGradientFill(grad);
    g.fillPath(p);

    g.setColour(scopeColor.withMultipliedBrightness(1.2f));
    g.strokePath(p, juce::PathStrokeType(1.2f));

    // --- CROSSHAIR (EJES X e Y TIPO SPAN) ---
    if (isHoveringFft)
    {
        float mx = juce::jlimit((float)bounds.getX(), (float)bounds.getRight(), fftCrosshairPos.x);
        float my = juce::jlimit((float)bounds.getY(), (float)bounds.getBottom(), fftCrosshairPos.y);

        g.setColour(juce::Colours::white.withAlpha(0.4f));
        g.drawVerticalLine((int)mx, (float)bounds.getY(), (float)bounds.getBottom());
        g.drawHorizontalLine((int)my, (float)bounds.getX(), (float)bounds.getRight());

        // Calcular Frecuencia logarítmica (asumiendo 20Hz a 20kHz)
        float normX = (mx - bounds.getX()) / bounds.getWidth();
        float logMin = std::log10(20.0f);
        float logMax = std::log10(20000.0f);
        float freq = std::pow(10.0f, logMin + normX * (logMax - logMin));

        float db = juce::jmap(my, bottomY, (float)bounds.getY(), -90.0f, 0.0f);

        juce::String freqStr;
        if (freq >= 1000.0f) freqStr = juce::String(freq / 1000.0f, 1) + " kHz";
        else freqStr = juce::String(freq, 0) + " Hz";

        juce::String dbStr = juce::String(db, 1) + " dB";

        juce::String tooltipText = freqStr + "  |  " + dbStr;
        int tw = 85;
        int th = 14;
        int tx = (int)mx + 6;
        int ty = (int)my - 18;

        if (tx + tw > bounds.getRight()) tx = (int)mx - tw - 6;
        if (ty < bounds.getY()) ty = (int)my + 6;

        juce::Rectangle<int> tooltipBox(tx, ty, tw, th);
        g.setColour(juce::Colours::black.withAlpha(0.85f));
        g.fillRoundedRectangle(tooltipBox.toFloat(), 3.0f);
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawRoundedRectangle(tooltipBox.toFloat(), 3.0f, 1.0f);

        g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
        g.setColour(juce::Colours::white);
        g.drawText(tooltipText, tooltipBox, juce::Justification::centred);
    }

    g.setFont(9.0f);
    g.setColour(juce::Colours::grey.withAlpha(0.8f));
    juce::String title = audioProcessor.btnFftMaster->get() ? "BALANCE (MASTER)" : "BALANCE (DEFAULT)";
    g.drawText(title, bounds.getX(), bounds.getY() + 4, bounds.getWidth(), 12, juce::Justification::centred);
}

void MiPrimerVSTAudioProcessorEditor::sliderValueChanged(juce::Slider* slider) {
    if (slider == &modeKnob) {
        int idx = (int)slider->getValue(); *audioProcessor.modeSelector = idx;
        updateModeText();
    }
    else if (slider == &vuCalibKnob) {
        *audioProcessor.vuCalibParam = (float)vuCalibKnob.getValue();
    }
}

// --- ACTUALIZAR MOUSE PARA CROSSHAIR ---
void MiPrimerVSTAudioProcessorEditor::mouseMove(const juce::MouseEvent& event) {
    auto virtualPos = event.getPosition().toFloat() / scaleFactor;
    juce::Rectangle<int> centralBounds = rectKnob.getUnion(rectStats).reduced(4);

    if (centralViewMode == 2 && centralBounds.contains(virtualPos.toInt())) {
        isHoveringFft = true;
        fftCrosshairPos = virtualPos;
        repaint(); // Redibuja rápido para sentir el mouse fluido
    }
    else if (isHoveringFft) {
        isHoveringFft = false;
        repaint();
    }
}

void MiPrimerVSTAudioProcessorEditor::mouseExit(const juce::MouseEvent& event) {
    if (isHoveringFft) {
        isHoveringFft = false;
        repaint();
    }
}

void MiPrimerVSTAudioProcessorEditor::mouseDown(const juce::MouseEvent& event) {
    auto virtualPos = event.getPosition().toFloat() / scaleFactor;

    if (centralViewMode == 1) {
        juce::Rectangle<int> vuBounds = rectKnob.getUnion(rectStats);
        if (vuBounds.contains(virtualPos.toInt())) {
            vuViewMode = (vuViewMode + 1) % 2;
            repaint();
            return;
        }
    }

    if (rectOscilloscope.contains(virtualPos.toInt())) {
        scopeViewMode = (scopeViewMode + 1) % 3;
        repaint();
    }
    else if (rectVectorscope.contains(virtualPos.toInt())) {
        if (audioProcessor.savedVectorscopeMode == 0) audioProcessor.savedVectorscopeMode = 1;
        else audioProcessor.savedVectorscopeMode = 0;
        repaint();
    }
}

void MiPrimerVSTAudioProcessorEditor::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
    auto virtualPos = event.getPosition().toFloat() / scaleFactor;

    if (rectOscilloscope.contains(virtualPos.toInt())) {
        if (scopeViewMode == 1) waveformEngine.modifyZoom(wheel.deltaY);
        else if (scopeViewMode == 2) proLScopeEngine.modifyZoom(wheel.deltaY);
    }
    else {
        AudioProcessorEditor::mouseWheelMove(event, wheel);
    }
}

void MiPrimerVSTAudioProcessorEditor::mouseDoubleClick(const juce::MouseEvent& event) {
    auto virtualPos = event.getPosition().toFloat() / scaleFactor;
    if (centralViewMode == 0 && (rectKnob.contains(virtualPos.toInt()) || rectStats.contains(virtualPos.toInt()))) {
        audioProcessor.resetLoudnessMeters();
    }
}

void MiPrimerVSTAudioProcessorEditor::drawLufsCentral(juce::Graphics& g, juce::Rectangle<int> bounds) {
    float shortLufs = audioProcessor.lufsShort.load();
    int centerY = bounds.getCentreY();
    g.setFont(juce::FontOptions(12.0f));
    g.setColour(juce::Colours::cyan.withAlpha(0.8f));
    g.drawText("LUFS Short", bounds.getX(), centerY - 32, bounds.getWidth(), 20, juce::Justification::centred);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(28.0f, juce::Font::bold));
    juce::String text = (shortLufs < -99) ? "- Inf" : juce::String(shortLufs, 1);
    g.drawText(text, bounds.getX(), centerY - 8, bounds.getWidth(), 30, juce::Justification::centred);
}

void MiPrimerVSTAudioProcessorEditor::drawLufsStats(juce::Graphics& g, juce::Rectangle<int> bounds) {
    float integrated = audioProcessor.lufsIntegrated.load();
    float range = audioProcessor.lufsRange.load();
    float peak = audioProcessor.lufsTruePeak.load();
    float peakDB = juce::Decibels::gainToDecibels(peak);
    int colW = bounds.getWidth() / 3; int x = bounds.getX(); int y = bounds.getY(); int h = bounds.getHeight();
    g.setFont(juce::FontOptions(10.0f));
    g.setColour(juce::Colours::grey);
    g.drawText("Range", x, y, colW, h / 2, juce::Justification::centred);
    g.drawText("dBTP", x + colW, y, colW, h / 2, juce::Justification::centred);
    g.drawText("Long", x + colW * 2, y, colW, h / 2, juce::Justification::centred);
    g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    g.setColour(juce::Colours::white);
    g.drawText(juce::String(range, 1), x, y + h / 2, colW, h / 2, juce::Justification::centred);
    juce::String peakText = (peakDB < -99) ? "-Inf" : juce::String(peakDB, 1);
    g.drawText(peakText, x + colW, y + h / 2, colW, h / 2, juce::Justification::centred);
    juce::String longText = (integrated < -99) ? "-Inf" : juce::String(integrated, 1);
    g.drawText(longText, x + colW * 2, y + h / 2, colW, h / 2, juce::Justification::centred);
}

juce::Colour MiPrimerVSTAudioProcessorEditor::getMeterColor(float t) {
    if (t < 0.6f) return meterLow.interpolatedWith(meterMid, t / 0.6f);
    else return meterMid.interpolatedWith(meterHigh, (t - 0.6f) / 0.4f);
}

juce::String MiPrimerVSTAudioProcessorEditor::getDbString(float val) {
    float db = (val > 0.00001f) ? juce::Decibels::gainToDecibels(val) : -100.0f;
    if (db < -90) return "-Inf"; return juce::String((int)std::round(db));
}

void MiPrimerVSTAudioProcessorEditor::drawMeter(juce::Graphics& g, int x, int y, int width, int height, float level, bool isRMS) {
    float db = (level > 0.0f) ? juce::Decibels::gainToDecibels(level) : -100.0f;

    // AQUÍ ESTABA EL ERROR: Faltaba el 0.0f antes del 1.0f en los argumentos de juce::jmap.
    float normalizedPos = juce::jlimit(0.0f, 1.0f, juce::jmap(db, -30.0f, isRMS ? -4.0f : 0.0f, 0.0f, 1.0f));

    if (isRMS) {
        g.setColour(juce::Colours::darkgrey.withAlpha(0.3f)); g.fillRect(x, y, width, height);
        int barHeight = (int)((float)height * normalizedPos); int barY = y + height - barHeight;
        if (barHeight > 0) {
            juce::ColourGradient grad(meterLow, (float)x, (float)(y + height), meterHigh, (float)x, (float)y, false);
            grad.addColour(0.6, meterMid); g.setGradientFill(grad); g.fillRect(x, barY, width, barHeight);
        }
    }
    else {
        const int blockHeight = 2; const int blockGap = 1; int numBlocks = height / (blockHeight + blockGap);
        int activeBlocks = (int)((float)numBlocks * normalizedPos);
        for (int i = 0; i < numBlocks; ++i) {
            int blockY = y + height - ((i + 1) * (blockHeight + blockGap)) + blockGap;
            if (i < activeBlocks) {
                float t = (float)i / (float)(numBlocks - 1);
                g.setColour(getMeterColor(t)); g.fillRect(x, blockY, width, blockHeight);
            }
            else {
                g.setColour(juce::Colours::darkgrey.withAlpha(0.3f)); g.fillRect(x, blockY, width, blockHeight);
            }
        }
    }
}

void MiPrimerVSTAudioProcessorEditor::drawOscilloscope(juce::Graphics& g, int x, int y, int width, int height) {
    if (scopeViewMode == 1) {
        waveformEngine.draw(g, juce::Rectangle<int>(x, y, width, height), scopeColor);
    }
    else if (scopeViewMode == 2) {
        proLScopeEngine.draw(g, juce::Rectangle<int>(x, y, width, height), scopeColor);
    }
    else {
        legacyScope.draw(g, x, y, width, height, scopeColor);
    }
}

void MiPrimerVSTAudioProcessorEditor::drawVectorscope(juce::Graphics& g, int x, int y, int width, int height)
{
    const int marginBot = 45;
    juce::Rectangle<int> scopeRect(x, y, width, height - marginBot);
    scopeRect = scopeRect.expanded(8, 0);

    juce::Rectangle<int> meterRect(x, y + height - marginBot, width, marginBot);

    {
        juce::Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(scopeRect.expanded(0, 15));

        if (audioProcessor.savedVectorscopeMode == 0)
        {
            float centerX = scopeRect.getCentreX(); float centerY = scopeRect.getCentreY();
            float baseScale = (float)scopeRect.getHeight() * 0.5f;
            float scaleX = baseScale * 1.4f; float scaleY = baseScale * 0.95f;
            LissajousBackground::draw(g, centerX, centerY, scaleX, scaleY);

            juce::Path vectorPath;
            bool firstPoint = true;
            const float* bufL = audioProcessor.scopeBufferL;
            const float* bufR = audioProcessor.scopeBufferR;

            for (int i = 0; i < MiPrimerVSTAudioProcessor::scopeSize; i += 2) {
                float l = bufL[i];
                float r = bufR[i];

                if (l > 1.0f) l = 1.0f; if (l < -1.0f) l = -1.0f;
                if (r > 1.0f) r = 1.0f; if (r < -1.0f) r = -1.0f;

                float side = (r - l);
                float mid = (l + r);

                float drawX = centerX + (side * scaleX * 0.707f);
                float drawY = centerY - (mid * scaleY * 0.707f);

                if (firstPoint) {
                    vectorPath.startNewSubPath(drawX, drawY);
                    firstPoint = false;
                }
                else {
                    vectorPath.lineTo(drawX, drawY);
                }
            }

            g.setColour(scopeColor);
            g.strokePath(vectorPath, juce::PathStrokeType(1.1f));
        }
        else if (audioProcessor.savedVectorscopeMode == 1)
        {
            g.setColour(juce::Colours::white.withAlpha(0.15f));
            float cx = scopeRect.getCentreX(); float by = (float)scopeRect.getBottom();

            float radius = std::min((float)scopeRect.getWidth() * 0.5f, (float)scopeRect.getHeight()) * 1.0f;

            juce::Path arcPath;
            arcPath.addArc(cx - radius, by - radius, radius * 2, radius * 2, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
            g.strokePath(arcPath, juce::PathStrokeType(1.0f));
            g.drawLine(cx, by, cx, by - radius, 1.0f);
            float p4 = juce::MathConstants<float>::pi * 0.25f;
            g.drawLine(cx, by, cx + std::sin(p4) * radius, by - std::cos(p4) * radius, 1.0f);
            g.drawLine(cx, by, cx - std::sin(p4) * radius, by - std::cos(p4) * radius, 1.0f);
            g.drawLine(cx - radius, by, cx + radius, by, 1.0f);

            polarEngine.draw(g, scopeRect, scopeColor);
        }
    }

    {
        juce::Graphics::ScopedSaveState save(g);
        g.reduceClipRegion(meterRect);
        int barH = 10; int barY = meterRect.getY() + (meterRect.getHeight() - barH) / 2 + 12;
        float midX = meterRect.getCentreX(); float barWidth = width - 20;

        g.setColour(juce::Colours::white);
        g.drawVerticalLine((int)midX, (float)barY, (float)(barY + barH));
        float corr = audioProcessor.currentCorrelation.load();
        corr = juce::jlimit(-1.0f, 1.0f, corr);
        g.setColour(corr >= 0 ? correlationPosColor : juce::Colours::red);
        float drawW = (std::abs(corr) * (barWidth * 0.5f));
        float drawX = (corr >= 0) ? midX : (midX - drawW);
        g.fillRect(drawX, (float)barY + 2, drawW, (float)barH - 4);
        g.setFont(juce::FontOptions(9.0f));
        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.drawText("-1", x + 10, barY, 20, barH, juce::Justification::centredLeft);
        g.drawText("+1", x + width - 30, barY, 20, barH, juce::Justification::centredRight);
    }
}