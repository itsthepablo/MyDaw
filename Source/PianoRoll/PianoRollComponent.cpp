#include "PianoRollComponent.h"
#include <cmath>

PianoRollComponent::PianoRollComponent()
{
    addAndMakeVisible(hBar); addAndMakeVisible(vBar);
    hBar.addListener(this); vBar.addListener(this);

    addAndMakeVisible(hZoomSlider); hZoomSlider.setRange(0.1, 10.0, 0.01); hZoomSlider.setValue(1.0);
    hZoomSlider.onValueChange = [this] { hZoom = (float)hZoomSlider.getValue(); updateScrollBars(); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos); repaint(); };

    addAndMakeVisible(vZoomSlider); vZoomSlider.setRange(0.5, 3.0, 0.01); vZoomSlider.setValue(1.0);
    vZoomSlider.onValueChange = [this] { vZoom = (float)vZoomSlider.getValue(); updateScrollBars(); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos); repaint(); };

    addAndMakeVisible(hZoomLabel); hZoomLabel.setText("H-Zoom", juce::dontSendNotification);
    addAndMakeVisible(vZoomLabel); vZoomLabel.setText("V-Zoom", juce::dontSendNotification);

    addAndMakeVisible(snapSelector);
    snapSelector.addItem("Main", 1); snapSelector.addItem("Line", 2); snapSelector.addItem("Cell", 3);
    snapSelector.addSeparator(); snapSelector.addItem("(none)", 4);
    snapSelector.addItem("1/6 step", 5); snapSelector.addItem("1/4 step", 6); snapSelector.addItem("1/3 step", 7); snapSelector.addItem("1/2 step", 8); snapSelector.addItem("Step", 9);
    snapSelector.addItem("1/6 beat", 10); snapSelector.addItem("1/4 beat", 11); snapSelector.addItem("1/3 beat", 12); snapSelector.addItem("1/2 beat", 13); snapSelector.addItem("Beat", 14); snapSelector.addItem("Bar", 15);
    snapSelector.setSelectedId(14);
    snapSelector.onChange = [this] { automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos); repaint(); };

    addAndMakeVisible(toolBtn); toolBtn.setButtonText("DIBUJAR"); toolBtn.setClickingTogglesState(true);
    toolBtn.onClick = [this] { currentTool = toolBtn.getToggleState() ? ToolMode::Select : ToolMode::Draw; toolBtn.setButtonText(toolBtn.getToggleState() ? "LAZO" : "DIBUJAR"); };

    addAndMakeVisible(linkAutoBtn); linkAutoBtn.setButtonText("LINK AUTO"); linkAutoBtn.setClickingTogglesState(true);
    linkAutoBtn.setToggleState(false, juce::dontSendNotification);

    addAndMakeVisible(rootNoteCombo); rootNoteCombo.addItemList({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1); rootNoteCombo.setSelectedItemIndex(0); rootNoteCombo.onChange = [this] { updateScale(); };

    addAndMakeVisible(scaleCombo);
    scaleCombo.addItem("Major (Ionian)", 1); scaleCombo.addItem("Major Bebop", 2); scaleCombo.addItem("Major Pentatonic", 3);
    scaleCombo.addItem("Minor Harmonic", 4); scaleCombo.addItem("Minor Melodic", 5); scaleCombo.addItem("Minor Natural (Aeolian)", 6);
    scaleCombo.addItem("Minor Pentatonic", 7); scaleCombo.addItem("Other Arabic", 8); scaleCombo.addItem("Other Blues", 9);
    scaleCombo.addItem("Other Dorian", 10); scaleCombo.addItem("Other Egyptian", 11); scaleCombo.addItem("Other Locrian", 12);
    scaleCombo.addItem("Other Lydian", 13); scaleCombo.addItem("Other Mixolydian", 14); scaleCombo.addItem("Other Phrygian", 15);
    scaleCombo.setSelectedItemIndex(0); scaleCombo.onChange = [this] { updateScale(); };

    addAndMakeVisible(automationEditor);

    setWantsKeyboardFocus(true); addKeyListener(this);
    startTimerHz(60);

    updateScrollBars(); vBar.setCurrentRangeStart(totalNotes * baseRowH / 2.0);
    updateScale();
}

PianoRollComponent::~PianoRollComponent() { stopTimer(); }

float PianoRollComponent::getLoopEndPos() const {
    double dynamicLoopEnd = 320.0;
    const auto& nts = getNotes();
    if (!nts.empty()) {
        double maxPx = 0.0; for (const auto& n : nts) { if (n.x + n.width > maxPx) maxPx = n.x + n.width; }
        dynamicLoopEnd = std::ceil((maxPx - 0.001) / 320.0) * 320.0; if (dynamicLoopEnd < 320.0) dynamicLoopEnd = 320.0;
    }
    return (float)dynamicLoopEnd;
}

bool PianoRollComponent::isInterestedInFileDrag(const juce::StringArray& files) {
    for (auto file : files) { if (file.endsWithIgnoreCase(".mid") || file.endsWithIgnoreCase(".midi")) return true; }
    return false;
}

void PianoRollComponent::filesDropped(const juce::StringArray& files, int x, int y) {
    if (externalNotes == nullptr) return;
    juce::File midiFileToLoad(files[0]); if (!midiFileToLoad.existsAsFile()) return;
    juce::FileInputStream stream(midiFileToLoad); juce::MidiFile midiFile; if (!midiFile.readFrom(stream)) return;
    float dropAbsX = std::max(0.0f, (x - keyW + (float)hBar.getCurrentRangeStart()) / hZoom);
    int startDropPixel = (int)(std::round(dropAbsX / getSnapPixels()) * getSnapPixels());
    short timeFormat = midiFile.getTimeFormat(); double ticksPerQuarterNote = (timeFormat > 0) ? timeFormat : 96.0;
    double pixelsPerTick = 80.0 / ticksPerQuarterNote;
    hasStateChanged = true; selectedNotes.clear();
    for (int t = 0; t < midiFile.getNumTracks(); ++t) {
        const juce::MidiMessageSequence* track = midiFile.getTrack(t);
        for (int i = 0; i < track->getNumEvents(); ++i) {
            auto msg = track->getEventPointer(i)->message;
            if (msg.isNoteOn()) {
                double startTick = msg.getTimeStamp(); double endTick = startTick + ticksPerQuarterNote;
                for (int j = i + 1; j < track->getNumEvents(); ++j) {
                    auto offMsg = track->getEventPointer(j)->message;
                    if (offMsg.isNoteOff() && offMsg.getNoteNumber() == msg.getNoteNumber()) { endTick = offMsg.getTimeStamp(); break; }
                }
                int noteX = startDropPixel + (int)(startTick * pixelsPerTick); int noteWidth = (int)((endTick - startTick) * pixelsPerTick);
                if (noteWidth < 1) noteWidth = 1; if (noteX >= 32.0 * 320.0) continue;
                int pitch = msg.getNoteNumber(); double freq = 440.0 * std::pow(2.0, (pitch - 69) / 12.0);
                externalNotes->push_back({ pitch, noteX, noteWidth, freq }); selectedNotes.insert((int)externalNotes->size() - 1);
            }
        }
    }
    commitHistory(); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos); repaint();
}

void PianoRollComponent::updateScrollBars() {
    double maxContentX = 32.0 * 320.0 * hZoom; double visibleW = (double)getWidth() - keyW - scrollBarSize;
    hBar.setRangeLimits(0.0, maxContentX); hBar.setCurrentRange(hBar.getCurrentRangeStart(), visibleW);
    double contentH = (double)totalNotes * getRowHeight(); int hbY = getHeight() - autoH - scrollBarSize; double visibleH = (double)hbY - (toolH + timelineH);
    vBar.setRangeLimits(0.0, contentH); vBar.setCurrentRange(vBar.getCurrentRangeStart(), visibleH);
}

void PianoRollComponent::commitHistory() { if (externalNotes == nullptr) return; if (currentHistoryIndex < (int)undoHistory.size() - 1) undoHistory.resize(currentHistoryIndex + 1); undoHistory.push_back(*externalNotes); currentHistoryIndex++; if (undoHistory.size() > 50) { undoHistory.erase(undoHistory.begin()); currentHistoryIndex--; } automationEditor.repaint(); }
void PianoRollComponent::undo() { if (externalNotes == nullptr || currentHistoryIndex <= 0) return; currentHistoryIndex--; *externalNotes = undoHistory[currentHistoryIndex]; selectedNotes.clear(); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos); repaint(); }
void PianoRollComponent::redo() { if (externalNotes == nullptr || currentHistoryIndex >= (int)undoHistory.size() - 1) return; currentHistoryIndex++; *externalNotes = undoHistory[currentHistoryIndex]; selectedNotes.clear(); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos); repaint(); }
void PianoRollComponent::copySelectedNotes() { if (externalNotes == nullptr) return; clipboard.clear(); for (int idx : selectedNotes) clipboard.push_back((*externalNotes)[idx]); }
void PianoRollComponent::pasteNotes() { if (externalNotes == nullptr || clipboard.empty()) return; int minX = 2000000000; for (auto& n : clipboard) if (n.x < minX) minX = n.x; selectedNotes.clear(); int offset = (int)playheadAbsPos - minX; for (auto n : clipboard) { n.x = std::max(0, n.x + offset); externalNotes->push_back(n); selectedNotes.insert((int)externalNotes->size() - 1); } commitHistory(); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos); }
void PianoRollComponent::quantizeNotes() { if (externalNotes == nullptr) return; double s = getSnapPixels(); std::vector<int> targets; if (selectedNotes.empty()) for (int i = 0; i < (int)externalNotes->size(); ++i) targets.push_back(i); else for (int idx : selectedNotes) targets.push_back(idx); if (targets.empty()) return; for (int idx : targets) { (*externalNotes)[idx].x = (int)(std::round((*externalNotes)[idx].x / s) * s); (*externalNotes)[idx].width = std::max((int)s, (int)(std::round((*externalNotes)[idx].width / s) * s)); } commitHistory(); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos); }

bool PianoRollComponent::isWhiteKey(int midiNote) { int n = midiNote % 12; return (n == 0 || n == 2 || n == 4 || n == 5 || n == 7 || n == 9 || n == 11); }
double PianoRollComponent::getSnapPixels() {
    double b = 80.0; double s = b / 4.0;
    switch (snapSelector.getSelectedId()) {
    case 1: case 2: return b; case 3: case 9: return s; case 4: return 1.0; case 5: return s / 6.0; case 6: return s / 4.0; case 7: return s / 3.0; case 8: return s / 2.0; case 10: return b / 6.0; case 11: return b / 4.0; case 12: return b / 3.0; case 13: return b / 2.0; case 14: return b; case 15: return b * 4.0; default: return b;
    }
}

int PianoRollComponent::yToPitch(int y) { return (totalNotes - 1) - (int)((y - (toolH + timelineH) + vBar.getCurrentRangeStart()) / getRowHeight()); }
int PianoRollComponent::pitchToY(int pitch) { return (int)(((totalNotes - 1) - pitch) * getRowHeight() + (toolH + timelineH) - vBar.getCurrentRangeStart()); }

int PianoRollComponent::getNoteAt(int x, int y) {
    if (externalNotes == nullptr) return -1;
    int hbY = getHeight() - autoH - scrollBarSize; if (x < keyW || x > getWidth() - scrollBarSize || y < toolH + timelineH || y > hbY) return -1;
    float absX = (x - keyW + (float)hBar.getCurrentRangeStart()) / hZoom;
    const auto& nts = getNotes();
    for (int i = (int)nts.size() - 1; i >= 0; --i) {
        int nY = pitchToY(nts[i].pitch); if (absX >= nts[i].x && absX <= nts[i].x + nts[i].width && y >= nY && y <= nY + getRowHeight()) return i;
    } return -1;
}

void PianoRollComponent::updateScale() {
    int r = rootNoteCombo.getSelectedItemIndex(); int s = scaleCombo.getSelectedItemIndex(); if (r < 0 || s < 0) return; currentRootNoteOffset = r;
    static const std::vector<std::vector<int>> allIntervals = { { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 4, 5, 7, 8, 9, 11 }, { 0, 2, 4, 7, 9 }, { 0, 2, 3, 5, 7, 8, 11 }, { 0, 2, 3, 5, 7, 9, 11 }, { 0, 2, 3, 5, 7, 8, 10 }, { 0, 3, 5, 7, 10 }, { 0, 1, 4, 5, 7, 8, 11 }, { 0, 3, 5, 6, 7, 10 }, { 0, 2, 3, 5, 7, 9, 10 }, { 0, 2, 5, 7, 10 }, { 0, 1, 3, 5, 6, 8, 10 }, { 0, 2, 4, 6, 7, 9, 11 }, { 0, 2, 4, 5, 7, 9, 10 }, { 0, 1, 3, 5, 7, 8, 10 } };
    currentScaleIntervals = allIntervals[s]; currentScaleNotesInOctave.clear(); for (int i : currentScaleIntervals) currentScaleNotesInOctave.push_back((currentRootNoteOffset + i) % 12); repaint();
}

bool PianoRollComponent::isNoteInScale(int midiPitch) const { int n = midiPitch % 12; for (int s : currentScaleNotesInOctave) if (s == n) return true; return false; }
juce::String PianoRollComponent::getNoteName(int midiPitch) const { static const juce::String n[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return n[midiPitch % 12] + juce::String(midiPitch / 12); }

void PianoRollComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkgrey.darker(0.9f));
    int hbY = getHeight() - autoH - scrollBarSize; int pH = hbY - (toolH + timelineH);
    float hS = (float)hBar.getCurrentRangeStart(); int gSY = toolH + timelineH;
    int visibleGridW = getWidth() - keyW - scrollBarSize;

    for (int i = 0; i < totalNotes; ++i) {
        int yPos = pitchToY(i); if (yPos < gSY - getRowHeight() || yPos > gSY + pH) continue;
        bool inS = isNoteInScale(i); g.setColour(inS ? juce::Colours::white.withAlpha(0.04f) : juce::Colours::black.withAlpha(0.25f));
        g.fillRect(keyW, yPos, visibleGridW, (int)getRowHeight());
        g.setColour(juce::Colours::black.withAlpha(0.2f)); g.drawHorizontalLine(yPos, (float)keyW, (float)getWidth() - scrollBarSize);
    }

    g.saveState();
    g.reduceClipRegion(keyW, gSY, visibleGridW, pH);

    if (currentMousePos.y >= gSY && currentMousePos.y < hbY && currentMousePos.x >= keyW) {
        int hoverPitch = yToPitch(currentMousePos.y); int hoverY = pitchToY(hoverPitch);
        g.setColour(juce::Colours::white.withAlpha(0.06f)); g.fillRect(keyW, hoverY, visibleGridW, (int)getRowHeight());
    }

    double vS = getSnapPixels(); if (vS * hZoom < 12.0) vS = 80.0;
    double limitX = 32.0 * 320.0;
    for (double i = 0; i <= limitX; i += vS) {
        int dx = (int)(i * hZoom) + keyW - (int)hS; if (dx > getWidth()) break;
        g.setColour(std::fmod(i, 320.0) < 0.1 ? juce::Colours::white.withAlpha(0.15f) : juce::Colours::white.withAlpha(0.05f));
        g.drawVerticalLine(dx, (float)gSY, (float)gSY + pH);
    }

    double dynamicLoopEnd = getLoopEndPos();
    int endLineX = (int)(dynamicLoopEnd * hZoom) + keyW - (int)hS;
    if (endLineX >= keyW && endLineX <= getWidth() - scrollBarSize) {
        g.setColour(juce::Colours::orange.withAlpha(0.6f)); g.drawVerticalLine(endLineX, (float)toolH, (float)gSY + pH);
    }

    const auto& nts = getNotes();
    for (int i = 0; i < (int)nts.size(); ++i) {
        int yPos = pitchToY(nts[i].pitch); int xPos = (int)(nts[i].x * hZoom) + keyW - (int)hS; int sW = (int)(nts[i].width * hZoom);
        if (xPos > getWidth() || xPos + sW < keyW) continue;
        auto nR = juce::Rectangle<int>(xPos, yPos + 1, sW - 1, (int)getRowHeight() - 2);

        bool inScale = isNoteInScale(nts[i].pitch);
        juce::Colour noteColor;
        if (selectedNotes.count(i) > 0) { noteColor = juce::Colours::yellow; }
        else if (!inScale) { noteColor = juce::Colour(220, 80, 80); }
        else { noteColor = juce::Colour(130, 90, 190); }

        g.setColour(noteColor); g.fillRect(nR);
        g.setColour(juce::Colours::black.withAlpha(0.5f)); g.drawRect(nR, 1.0f);
        g.setColour(juce::Colours::black); g.setFont(juce::Font(std::max(8.0f, getRowHeight() * 0.6f), juce::Font::bold));
        g.drawText(getNoteName(nts[i].pitch), nR, juce::Justification::centred, true);
    }

    for (const auto& anim : animations) {
        int yPos = pitchToY(anim.pitch); int xPos = (int)(anim.x * hZoom) + keyW - (int)hS; int sW = (int)(anim.width * hZoom);
        if (xPos > getWidth() || xPos + sW < keyW) continue;
        if (anim.isDeleting) {
            float scale = 0.6f + (anim.alpha * 0.4f); float cx = xPos + (sW / 2.0f); float cy = yPos + (getRowHeight() / 2.0f);
            float curW = sW * scale; float curH = (getRowHeight() - 2) * scale;
            juce::Rectangle<float> r(cx - curW / 2.0f, cy - curH / 2.0f, curW, curH);
            g.setColour(juce::Colour(130, 90, 190).withAlpha(anim.alpha)); g.fillRoundedRectangle(r, 2.0f);
        }
        else {
            juce::Rectangle<float> r(xPos, yPos + 1, sW - 1, getRowHeight() - 2);
            g.setColour(juce::Colours::white.withAlpha(anim.alpha * 0.8f)); g.fillRoundedRectangle(r, 2.0f);
        }
    }

    g.restoreState();

    if (isSelecting) { auto r = juce::Rectangle<int>(selectionStart, selectionEnd); g.setColour(juce::Colours::orange.withAlpha(0.15f)); g.fillRect(r); g.setColour(juce::Colours::orange.withAlpha(0.8f)); g.drawRect(r, 1.5f); }
    for (int i = 0; i < totalNotes; ++i) {
        int yPos = pitchToY(i); if (yPos < gSY - getRowHeight() || yPos > gSY + pH) continue;
        int n = i % 12; bool isB = (n == 1 || n == 3 || n == 6 || n == 8 || n == 10); bool wS = isNoteInScale(i);
        g.setColour(isB ? juce::Colours::black : (wS ? juce::Colours::white : juce::Colours::white.withAlpha(0.5f)));
        g.fillRect(0, yPos, keyW, (int)getRowHeight() - 1);
        g.setColour(wS ? juce::Colours::black : (isB ? juce::Colours::white.withAlpha(0.4f) : juce::Colours::darkgrey));
        g.setFont(juce::Font(std::max(8.0f, getRowHeight() * 0.6f), isWhiteKey(i) ? juce::Font::bold : juce::Font::plain));
        g.drawText(getNoteName(i), 5, yPos, keyW - 10, (int)getRowHeight(), juce::Justification::centredRight);
    }
    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.fillRect(keyW, toolH, visibleGridW, timelineH);
    g.setColour(juce::Colours::cyan.withAlpha(0.7f)); g.setFont(juce::Font("Sans-Serif", 12.0f, juce::Font::bold));
    for (double i = 0; i <= limitX; i += 320) {
        int dx = (int)(i * hZoom) + keyW - (int)hS; if (dx < keyW) continue; if (dx > getWidth() - scrollBarSize) break;
        g.drawText(juce::String((int)i / 320 + 1), dx + 4, toolH, 40, timelineH, juce::Justification::centredLeft);
    }

    if (endLineX >= keyW && endLineX <= getWidth() - scrollBarSize) {
        juce::Path loopTri; loopTri.addTriangle((float)endLineX, (float)toolH, (float)endLineX - 6.0f, (float)toolH, (float)endLineX, (float)gSY);
        g.setColour(juce::Colours::orange); g.fillPath(loopTri);
    }

    int phX = (int)(playheadAbsPos * hZoom) + keyW - (int)hS;
    if (phX >= keyW && phX <= getWidth() - scrollBarSize) {
        g.setColour(juce::Colours::red.withAlpha(0.8f)); g.drawVerticalLine(phX, (float)toolH, (float)hbY);
        juce::Path t; t.addTriangle((float)phX - 6, (float)toolH, (float)phX + 6, (float)toolH, (float)phX, (float)(toolH + timelineH));
        g.setColour(juce::Colours::lawngreen); g.fillPath(t);
    }
    g.setColour(juce::Colours::black); g.fillRect(0, hbY - 4, getWidth() - scrollBarSize, 4);
    g.setColour(juce::Colours::grey.withAlpha(0.5f)); g.drawHorizontalLine(hbY - 1, 0.0f, (float)getWidth());
    g.setColour(juce::Colours::black.withAlpha(0.9f)); g.fillRect(0, 0, getWidth(), toolH);
}

void PianoRollComponent::resized() {
    snapSelector.setBounds(10, 10, 100, 30);
    toolBtn.setBounds(120, 10, 100, 30); // Reajustado para ocupar el hueco del BPM
    linkAutoBtn.setBounds(230, 10, 100, 30);
    hZoomLabel.setBounds(345, 15, 50, 20); hZoomSlider.setBounds(395, 15, 80, 20);
    vZoomLabel.setBounds(485, 15, 50, 20); vZoomSlider.setBounds(535, 15, 80, 20);
    rootNoteCombo.setBounds(getWidth() - 320, 10, 50, 30); scaleCombo.setBounds(getWidth() - 260, 10, 240, 30);
    int hbY = getHeight() - autoH - scrollBarSize; automationEditor.setBounds(0, getHeight() - autoH, getWidth(), autoH); hBar.setBounds(keyW, hbY, getWidth() - keyW - scrollBarSize, scrollBarSize); vBar.setBounds(getWidth() - scrollBarSize, toolH + timelineH, scrollBarSize, hbY - (toolH + timelineH));
    updateScrollBars(); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos);
}

void PianoRollComponent::timerCallback() {
    bool needsRepaint = isPlaying;
    for (int i = (int)animations.size() - 1; i >= 0; --i) { animations[i].alpha -= 0.08f; if (animations[i].alpha <= 0.0f) animations.erase(animations.begin() + i); needsRepaint = true; }
    if (isPlaying) { automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos); }
    if (needsRepaint) repaint();
}

void PianoRollComponent::scrollBarMoved(juce::ScrollBar* sb, double) { if (sb == &hBar) automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos); repaint(); }

void PianoRollComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
    if (event.mods.isCtrlDown() || event.mods.isCommandDown()) {
        float timeAtMouse = (event.x - keyW + (float)hBar.getCurrentRangeStart()) / hZoom;
        float factor = (wheel.deltaY > 0) ? 1.15f : 0.85f;
        hZoomSlider.setValue(hZoomSlider.getValue() * factor); hZoom = (float)hZoomSlider.getValue(); updateScrollBars();
        hBar.setCurrentRangeStart(timeAtMouse * hZoom - (event.x - keyW));
    }
    else if (event.mods.isAltDown()) {
        double absY = (event.y - (toolH + timelineH)) + vBar.getCurrentRangeStart();
        double noteOffset = absY / getRowHeight();
        float factor = (wheel.deltaY > 0) ? 1.15f : 0.85f;
        vZoomSlider.setValue(vZoomSlider.getValue() * factor); vZoom = (float)vZoomSlider.getValue(); updateScrollBars();
        double newScrollStart = (noteOffset * getRowHeight()) - (event.y - (toolH + timelineH));
        newScrollStart = juce::jlimit(0.0, std::max(0.0, vBar.getRangeLimit().getEnd() - vBar.getCurrentRangeSize()), newScrollStart);
        vBar.setCurrentRangeStart(newScrollStart);
    }
    else {
        if (std::abs(wheel.deltaX) > std::abs(wheel.deltaY) || event.mods.isShiftDown()) { hBar.mouseWheelMove(event, wheel); }
        else {
            double speedMultiplier = 400.0;
            double newStart = vBar.getCurrentRangeStart() - (wheel.deltaY * speedMultiplier);
            newStart = juce::jlimit(0.0, std::max(0.0, vBar.getRangeLimit().getEnd() - vBar.getCurrentRangeSize()), newStart);
            vBar.setCurrentRangeStart(newStart);
        }
    }
    automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos);
    repaint();
}

void PianoRollComponent::mouseMove(const juce::MouseEvent& event) {
    currentMousePos = event.getPosition();
    int hbY = getHeight() - autoH - scrollBarSize; if (std::abs(event.y - hbY) <= 4) { setMouseCursor(juce::MouseCursor::UpDownResizeCursor); return; }
    int idx = getNoteAt(event.x, event.y); const auto& nts = getNotes();
    if (idx != -1 && event.x > (nts[idx].x * hZoom + keyW - (float)hBar.getCurrentRangeStart() + nts[idx].width * hZoom - 10)) setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else if (idx != -1) setMouseCursor(juce::MouseCursor::DraggingHandCursor); else setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

void PianoRollComponent::mouseDown(const juce::MouseEvent& event) {
    if (externalNotes == nullptr) return;
    isAltDragging = false; hasStateChanged = false; int hbY = getHeight() - autoH - scrollBarSize;
    if (std::abs(event.y - hbY) <= 4) { isResizingAutoPanel = true; return; }
    float absX = std::max(0.0f, (event.x - keyW + (float)hBar.getCurrentRangeStart()) / hZoom);

    if (event.x < keyW) {
        if (event.y >= toolH + timelineH && event.y < hbY) { previewPitch = yToPitch(event.y); isPreviewingNote = true; }
        return;
    }

    if (event.y >= toolH && event.y < toolH + timelineH) { isDraggingPlayhead = true; playheadAbsPos = (float)(std::round(absX / getSnapPixels()) * getSnapPixels()); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos); repaint(); return; }
    if (event.x > getWidth() - scrollBarSize || event.y < toolH + timelineH || event.y >= hbY) return;

    int p = yToPitch(event.y); double f = 440.0 * std::pow(2.0, (p - 69) / 12.0);

    if (event.mods.isRightButtonDown()) {
        int clickedIdx = getNoteAt(event.x, event.y);
        if (clickedIdx != -1) {
            hasStateChanged = true;
            if (selectedNotes.count(clickedIdx)) {
                std::vector<int> toD(selectedNotes.begin(), selectedNotes.end()); std::sort(toD.rbegin(), toD.rend());
                for (int i : toD) { animations.push_back({ (*externalNotes)[i].pitch, (*externalNotes)[i].x, (*externalNotes)[i].width, 1.0f, true }); externalNotes->erase(externalNotes->begin() + i); }
            }
            else {
                animations.push_back({ (*externalNotes)[clickedIdx].pitch, (*externalNotes)[clickedIdx].x, (*externalNotes)[clickedIdx].width, 1.0f, true }); externalNotes->erase(externalNotes->begin() + clickedIdx);
            }
            selectedNotes.clear(); repaint(); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos);
        }
        return;
    }

    int clickedIdx = getNoteAt(event.x, event.y);
    if (clickedIdx != -1) {
        if (!selectedNotes.count(clickedIdx)) { if (!event.mods.isShiftDown()) selectedNotes.clear(); selectedNotes.insert(clickedIdx); }
        dragStates.clear(); for (int i : selectedNotes) dragStates.push_back({ i, (*externalNotes)[i].x, (*externalNotes)[i].pitch, (*externalNotes)[i].width });
        isResizing = (event.x > ((*externalNotes)[clickedIdx].x * hZoom + keyW - (float)hBar.getCurrentRangeStart() + (*externalNotes)[clickedIdx].width * hZoom - 10));
        dragStartAbsX = absX; dragStartPitch = p; previewPitch = p; isPreviewingNote = true;
        if (linkAutoBtn.getToggleState() && !isResizing) { automationEditor.grabNodesUnderNotes(selectedNotes); }
    }
    else {
        if (currentTool == ToolMode::Select || event.mods.isCtrlDown() || event.mods.isCommandDown()) { isSelecting = true; selectionStart = event.getPosition(); selectionEnd = event.getPosition(); if (!event.mods.isShiftDown()) selectedNotes.clear(); }
        else {
            hasStateChanged = true; selectedNotes.clear();
            int fx = (int)(std::floor(absX / getSnapPixels()) * getSnapPixels()); if (fx >= 32.0 * 320.0) return;
            externalNotes->push_back({ p, fx, lastNoteWidth, f }); animations.push_back({ p, fx, lastNoteWidth, 1.0f, false });
            int ni = (int)externalNotes->size() - 1; selectedNotes.insert(ni); dragStates.clear(); dragStates.push_back({ ni, fx, p, lastNoteWidth }); isResizing = true; dragStartAbsX = absX; dragStartPitch = p; previewPitch = p; isPreviewingNote = true;
        }
    }
    repaint(); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos);
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& event) {
    if (externalNotes == nullptr) return;
    currentMousePos = event.getPosition();
    if (isResizingAutoPanel) { autoH = juce::jlimit(50, getHeight() - 200, getHeight() - event.y - scrollBarSize); resized(); return; }
    float absX = std::max(0.0f, (event.x - keyW + (float)hBar.getCurrentRangeStart()) / hZoom);
    if (isDraggingPlayhead) { playheadAbsPos = (float)(std::round(absX / getSnapPixels()) * getSnapPixels()); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos); repaint(); return; }

    int hbY = getHeight() - autoH - scrollBarSize;
    if (event.x < keyW) { if (event.y >= toolH + timelineH && event.y < hbY) { previewPitch = yToPitch(event.y); isPreviewingNote = true; } return; }

    if (isSelecting) { selectionEnd = event.getPosition(); selectionEnd.y = std::max(toolH + timelineH, std::min(getHeight() - autoH - scrollBarSize, selectionEnd.y)); auto selR = juce::Rectangle<int>(selectionStart, selectionEnd); if (!event.mods.isShiftDown()) selectedNotes.clear(); for (int i = 0; i < (int)externalNotes->size(); ++i) { juce::Rectangle<int> nR((int)((*externalNotes)[i].x * hZoom) + keyW - (int)hBar.getCurrentRangeStart(), pitchToY((*externalNotes)[i].pitch), (int)((*externalNotes)[i].width * hZoom), (int)getRowHeight()); if (selR.intersects(nR)) selectedNotes.insert(i); } repaint(); return; }
    if (selectedNotes.empty()) return;
    if (event.mods.isAltDown() && !isAltDragging && !isResizing) { hasStateChanged = true; isAltDragging = true; std::set<int> newS; for (int i : selectedNotes) { externalNotes->push_back((*externalNotes)[i]); newS.insert((int)externalNotes->size() - 1); } selectedNotes = newS; dragStates.clear(); for (int i : selectedNotes) dragStates.push_back({ i, (*externalNotes)[i].x, (*externalNotes)[i].pitch, (*externalNotes)[i].width }); }
    hasStateChanged = true; int snappedDX = (int)(std::round((absX - dragStartAbsX) / getSnapPixels()) * getSnapPixels()); int dP = yToPitch(event.y) - dragStartPitch;
    bool canMP = true; if (!isResizing) for (auto& s : dragStates) if (s.startPitch + dP < 0 || s.startPitch + dP > 127) canMP = false;
    for (auto& s : dragStates) {
        if (isResizing) { (*externalNotes)[s.index].width = std::max((int)getSnapPixels(), (int)(std::round((s.startWidth + (absX - dragStartAbsX)) / getSnapPixels()) * getSnapPixels())); lastNoteWidth = (*externalNotes)[s.index].width; }
        else { (*externalNotes)[s.index].x = juce::jlimit(0, (int)(32.0 * 320.0), s.startX + snappedDX); if (canMP && dP != 0) { int newP = s.startPitch + dP; if ((*externalNotes)[s.index].pitch != newP) { (*externalNotes)[s.index].pitch = newP; (*externalNotes)[s.index].frequency = 440.0 * std::pow(2.0, (newP - 69) / 12.0); previewPitch = newP; isPreviewingNote = true; } } }
    }
    if (linkAutoBtn.getToggleState() && snappedDX != 0 && !isResizing) { automationEditor.moveLinkedNodes((float)snappedDX); }
    repaint(); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos);
}

void PianoRollComponent::mouseUp(const juce::MouseEvent&) {
    if (isResizingAutoPanel) { isResizingAutoPanel = false; return; }
    isPreviewingNote = false; isDraggingPlayhead = false;
    if (hasStateChanged) { commitHistory(); hasStateChanged = false; }
    if (isSelecting) { isSelecting = false; repaint(); }
    automationEditor.clearLinkedNodes();
}

bool PianoRollComponent::keyPressed(const juce::KeyPress& key, juce::Component*) {
    auto mods = key.getModifiers(); bool isC = mods.isCtrlDown() || mods.isCommandDown(); bool isS = mods.isShiftDown();
    if (isC) {
        if (key.getKeyCode() == 'z' || key.getKeyCode() == 'Z') { if (isS) redo(); else undo(); return true; }
        if (key.getKeyCode() == 'y' || key.getKeyCode() == 'Y') { redo(); return true; }
        if (key.getKeyCode() == 'c' || key.getKeyCode() == 'C') { copySelectedNotes(); return true; }
        if (key.getKeyCode() == 'v' || key.getKeyCode() == 'V') { pasteNotes(); return true; }
        if (key.getKeyCode() == 'q' || key.getKeyCode() == 'Q') { quantizeNotes(); return true; }
    }
    if (key.getKeyCode() == juce::KeyPress::spaceKey) { isPlaying = !isPlaying; repaint(); return true; }
    if (key.getKeyCode() == juce::KeyPress::deleteKey || key.getKeyCode() == juce::KeyPress::backspaceKey) {
        if (!selectedNotes.empty() && externalNotes != nullptr) {
            hasStateChanged = true; std::vector<int> toD(selectedNotes.begin(), selectedNotes.end()); std::sort(toD.rbegin(), toD.rend());
            for (int i : toD) { animations.push_back({ (*externalNotes)[i].pitch, (*externalNotes)[i].x, (*externalNotes)[i].width, 1.0f, true }); externalNotes->erase(externalNotes->begin() + i); }
            selectedNotes.clear(); commitHistory(); repaint(); automationEditor.updateView((float)hBar.getCurrentRangeStart(), hZoom, (float)vBar.getCurrentRangeStart(), vZoom, (float)getSnapPixels(), playheadAbsPos);
        } return true;
    }
    return false;
}