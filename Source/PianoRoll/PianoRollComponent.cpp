#include "PianoRollComponent.h"
#include "../Playlist/View/PlaylistGridRenderer.h"
#include <cmath>
#include "../Bridge/PythonBridge.h"

PianoRollComponent::PianoRollComponent()
{
    addAndMakeVisible(hBar); addAndMakeVisible(vBar);
    
    hBar.onScrollMoved = [this](double newStart) {
        repaint();
    };

    hBar.onZoomChanged = [this](double newZoom, double newStart) {
        hZoom = newZoom;
        updateScrollBars();
        repaint();
    };

    hBar.onDrawMinimap = [this](juce::Graphics& g, juce::Rectangle<int> b) {
        drawMinimap(g, b);
    };

    vBar.onScrollMoved = [this](double newStart) {
        repaint();
    };

    addAndMakeVisible(snapSelector);
    snapSelector.addItem("Main", 1); snapSelector.addItem("Line", 2); snapSelector.addItem("Cell", 3);
    snapSelector.addSeparator(); snapSelector.addItem("(none)", 4);
    snapSelector.addItem("1/6 step", 5); snapSelector.addItem("1/4 step", 6); snapSelector.addItem("1/3 step", 7); snapSelector.addItem("1/2 step", 8); snapSelector.addItem("Step", 9);
    snapSelector.addItem("1/6 beat", 10); snapSelector.addItem("1/4 beat", 11); snapSelector.addItem("1/3 beat", 12); snapSelector.addItem("1/2 beat", 13); snapSelector.addItem("Beat", 14); snapSelector.addItem("Bar", 15);
    snapSelector.setSelectedId(14);
    snapSelector.onChange = [this] { repaint(); };

    addAndMakeVisible(toolBtn); toolBtn.setButtonText("DIBUJAR"); toolBtn.setClickingTogglesState(true);
    toolBtn.onClick = [this] { currentTool = toolBtn.getToggleState() ? ToolMode::Select : ToolMode::Draw; toolBtn.setButtonText(toolBtn.getToggleState() ? "LAZO" : "DIBUJAR"); };

    addAndMakeVisible(linkAutoBtn); linkAutoBtn.setButtonText("LINK AUTO"); linkAutoBtn.setClickingTogglesState(true);
    linkAutoBtn.setToggleState(false, juce::dontSendNotification);

    addAndMakeVisible(pyHumanizeBtn); pyHumanizeBtn.setButtonText("HUMANIZE (PY)");
    pyHumanizeBtn.onClick = [this] { processPythonHumanize(); };

    addAndMakeVisible(chordGeneratorBtn); chordGeneratorBtn.setButtonText("TEXT TO CHORD");
    chordGeneratorBtn.onClick = [this] { processTextToChord(); };

    addAndMakeVisible(rootNoteCombo); rootNoteCombo.addItemList({ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 1); rootNoteCombo.setSelectedItemIndex(0); rootNoteCombo.onChange = [this] { updateScale(); };

    addAndMakeVisible(scaleCombo);
    scaleCombo.addItem("Major (Ionian)", 1); scaleCombo.addItem("Major Bebop", 2); scaleCombo.addItem("Major Pentatonic", 3);
    scaleCombo.addItem("Minor Harmonic", 4); scaleCombo.addItem("Minor Melodic", 5); scaleCombo.addItem("Minor Natural (Aeolian)", 6);
    scaleCombo.addItem("Minor Pentatonic", 7); scaleCombo.addItem("Other Arabic", 8); scaleCombo.addItem("Other Blues", 9);
    scaleCombo.addItem("Other Dorian", 10); scaleCombo.addItem("Other Egyptian", 11); scaleCombo.addItem("Other Locrian", 12);
    scaleCombo.addItem("Other Lydian", 13); scaleCombo.addItem("Other Mixolydian", 14); scaleCombo.addItem("Other Phrygian", 15);
    scaleCombo.setSelectedItemIndex(0); scaleCombo.onChange = [this] { updateScale(); };

    setWantsKeyboardFocus(true); addKeyListener(this);
    startTimerHz(60);

    updateScrollBars(); vBar.setCurrentRange(totalNotes * baseRowH / 2.0, 400.0);
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

void PianoRollComponent::processPythonHumanize()
{
    if (activeClip == nullptr) return;

    auto& notes = activeClip->getNotes();
    if (notes.empty()) return;

    // Ejecutar el puente
    if (PythonBridge::executeScript("humanize.py", notes))
    {
        commitHistory();
        repaint();
    }
}

void PianoRollComponent::processTextToChord()
{
    if (activeClip == nullptr) return;

    // Crear la ventana emergente asíncrona dedicada (JUCE 8 compatible)
    auto* aw = new juce::AlertWindow("TEXT TO CHORD", 
                                   "Introduce la progresión de acordes deseada:", 
                                   juce::MessageBoxIconType::NoIcon);

    aw->addTextEditor("chordProg", "Cmaj7 Am7 Fmaj7 G7", "Ej: Dm7 G7 Cmaj7");
    aw->addButton("Generar ahora", 1, juce::KeyPress(juce::KeyPress::returnKey));
    aw->addButton("Cancelar", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    // Lanzar de forma asíncrona para no bloquear el DAW
    aw->enterModalState(true, juce::ModalCallbackFunction::create([this, aw](int result)
    {
        if (result == 1) // El usuario pulsó "Generar ahora"
        {
            juce::String progression = aw->getTextEditorContents("chordProg");
            
            if (activeClip != nullptr)
            {
                auto& notes = activeClip->getNotes();
                if (PythonBridge::executeScript("text_to_chord.py", notes, progression))
                {
                    commitHistory();
                    repaint();
                }
            }
        }
        
        // Limpieza de la ventana
        delete aw;

    }), true);
}

void PianoRollComponent::filesDropped(const juce::StringArray& files, int x, int y) {
    if (activeClip == nullptr) return;
    juce::File midiFileToLoad(files[0]); if (!midiFileToLoad.existsAsFile()) return;
    juce::FileInputStream stream(midiFileToLoad); juce::MidiFile midiFile; if (!midiFile.readFrom(stream)) return;
    double dropAbsX = std::max(0.0, (double)(x - keyW + hBar.getCurrentRangeStart()) / hZoom);
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
                activeClip->getNotes().push_back({ pitch, noteX, noteWidth, freq }); selectedNotes.insert((int)activeClip->getNotes().size() - 1);
            }
        }
    }
    commitHistory(); repaint();
}

void PianoRollComponent::updateScrollBars() {
    double effectiveLen = getContentLengthTicks();
    double maxContentX = effectiveLen * hZoom; 
    double visibleW = (double)getWidth() - keyW - vBarWidth;
    
    double currentHStart = hBar.getCurrentRangeStart();

    // Sincronizar ancho del clip con contenido y resetear si está vacío
    if (activeClip != nullptr) {
        if (activeClip->getWidth() < effectiveLen) activeClip->setWidth(effectiveLen);
        
        if (getNotes().empty()) {
            activeClip->setWidth(1280.0); // Resetear loop a 4 compases
        }
    }

    double minHZoom = visibleW / effectiveLen;
    if (hZoom < (float)minHZoom) hZoom = (float)minHZoom;

    hBar.setZoomContext((double)hZoom, effectiveLen);
    if (activeClip != nullptr) {
        hBar.setRangeLimits(0.0, getContentLengthTicks() * hZoom);
        vBar.setRangeLimits(0.0, totalNotes * getRowHeight());
    }
    double contentH = (double)totalNotes * getRowHeight(); 
    int hbY = getHeight() - navigatorH; 
    double visibleH = (double)hbY - (toolH + timelineH);
    vBar.setRangeLimits(0.0, contentH); 
    vBar.setCurrentRange(vBar.getCurrentRangeStart(), visibleH);
    hBar.setCurrentRange(currentHStart, visibleW);
}

void PianoRollComponent::commitHistory() {
    if (activeClip == nullptr) return;
    if (currentHistoryIndex < (int)undoHistory.size() - 1) undoHistory.resize(currentHistoryIndex + 1);
    undoHistory.push_back(activeClip->getNotes());
    currentHistoryIndex++;
    if (undoHistory.size() > 50) { undoHistory.erase(undoHistory.begin()); currentHistoryIndex--; }
    hBar.repaint();
    updateScrollBars();
    notifyPatternEdited();
}

void PianoRollComponent::undo() { if (activeClip == nullptr || currentHistoryIndex <= 0) return; currentHistoryIndex--; activeClip->getNotes() = undoHistory[currentHistoryIndex]; selectedNotes.clear(); notifyPatternEdited(); hBar.repaint(); repaint(); }
void PianoRollComponent::redo() { if (activeClip == nullptr || currentHistoryIndex >= (int)undoHistory.size() - 1) return; currentHistoryIndex++; activeClip->getNotes() = undoHistory[currentHistoryIndex]; selectedNotes.clear(); notifyPatternEdited(); hBar.repaint(); repaint(); }
void PianoRollComponent::copySelectedNotes() { if (activeClip == nullptr) return; clipboard.clear(); for (int idx : selectedNotes) clipboard.push_back(activeClip->getNotes()[idx]); }
void PianoRollComponent::pasteNotes() { if (activeClip == nullptr || clipboard.empty()) return; int minX = 2000000000; for (auto& n : clipboard) if (n.x < minX) minX = n.x; selectedNotes.clear(); int offset = (int)playheadAbsPos - minX; for (auto n : clipboard) { n.x = std::max(0, n.x + offset); activeClip->getNotes().push_back(n); selectedNotes.insert((int)activeClip->getNotes().size() - 1); } commitHistory(); }
void PianoRollComponent::quantizeNotes() { if (activeClip == nullptr) return; double s = getSnapPixels(); std::vector<int> targets; if (selectedNotes.empty()) for (int i = 0; i < (int)activeClip->getNotes().size(); ++i) targets.push_back(i); else for (int idx : selectedNotes) targets.push_back(idx); if (targets.empty()) return; for (int idx : targets) { activeClip->getNotes()[idx].x = (int)(std::round(activeClip->getNotes()[idx].x / s) * s); activeClip->getNotes()[idx].width = std::max((int)s, (int)(std::round(activeClip->getNotes()[idx].width / s) * s)); } commitHistory(); }

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
    if (activeClip == nullptr) return -1;
    int hbY = getHeight() - navigatorH; if (x < keyW || x > getWidth() - vBarWidth || y < toolH + timelineH || y > hbY) return -1;
    double hS = hBar.getCurrentRangeStart();
    double absX = (double)(x - keyW + hS) / hZoom;
    const auto& nts = getNotes();
    for (int i = (int)nts.size() - 1; i >= 0; --i) {
        if (nts[i].pitch < 0 || nts[i].pitch >= totalNotes) continue;
        int nY = pitchToY(nts[i].pitch); 
        if (absX >= nts[i].x && absX <= nts[i].x + nts[i].width && y >= nY && y <= nY + getRowHeight()) return i;
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
    int hbY = getHeight() - navigatorH; int pH = hbY - (toolH + timelineH);
    double hS = hBar.getCurrentRangeStart(); int gSY = toolH + timelineH;
    int visibleGridW = getWidth() - keyW - vBarWidth;

    for (int i = 0; i < totalNotes; ++i) {
        int yPos = pitchToY(i); if (yPos < gSY - getRowHeight() || yPos > gSY + pH) continue;
        bool inS = isNoteInScale(i); g.setColour(inS ? juce::Colours::white.withAlpha(0.04f) : juce::Colours::black.withAlpha(0.25f));
        g.fillRect(keyW, yPos, visibleGridW, (int)getRowHeight());
        g.setColour(juce::Colours::black.withAlpha(0.2f)); g.drawHorizontalLine(yPos, (float)keyW, (float)getWidth() - vBarWidth);
    }

    g.saveState();
    g.reduceClipRegion(keyW, gSY, visibleGridW, pH);

    if (currentMousePos.y >= gSY && currentMousePos.y < hbY && currentMousePos.x >= keyW) {
        int hoverPitch = yToPitch(currentMousePos.y); 
        if (hoverPitch >= 0 && hoverPitch < totalNotes) {
            int hoverY = pitchToY(hoverPitch);
            g.setColour(juce::Colours::white.withAlpha(0.06f)); g.fillRect(keyW, hoverY, visibleGridW, (int)getRowHeight());
        }
    }

    double visualSnap = getSnapPixels(); if (visualSnap * hZoom < 12.0) visualSnap = 80.0;
    double limitX = getContentLengthTicks();
    for (double i = 0; i <= limitX; i += visualSnap) {
        int dx = (int)(i * hZoom - hS) + keyW; 
        if (dx < keyW - 100) continue; if (dx > getWidth() - vBarWidth + 100) break;
        g.setColour(std::fmod(i, 320.0) < 0.1 ? juce::Colours::white.withAlpha(0.15f) : juce::Colours::white.withAlpha(0.05f));
        g.drawVerticalLine(dx, (float)gSY, (float)gSY + pH);
    }

    double dynamicLoopEnd = getLoopEndPos();
    int endLineX = (int)(dynamicLoopEnd * hZoom - hS) + keyW;
    if (endLineX >= keyW && endLineX <= getWidth() - vBarWidth) {
        g.setColour(juce::Colours::orange.withAlpha(0.6f)); g.drawVerticalLine(endLineX, (float)toolH, (float)gSY + pH);
    }

    // --- SOMBRA DINÁMICA POR ANCHO DE CLIP (Sincronizado) ---
    // La zona brillante ahora sigue exactamente el ancho del clip en la Playlist
    float finalWidth = (activeClip != nullptr) ? activeClip->getWidth() : 1280.0f;

    int contentEndX = (int)(finalWidth * hZoom - hS) + keyW;
    if (contentEndX < getWidth() - vBarWidth) {
        g.setColour(juce::Colours::black.withAlpha(0.4f));
        g.fillRect(std::max(keyW, contentEndX), gSY, (getWidth() - vBarWidth) - std::max(keyW, contentEndX), pH);
    }

    const auto& nts = getNotes();
    for (int i = 0; i < (int)nts.size(); ++i) {
        int yPos = pitchToY(nts[i].pitch); 
        int xPos = (int)(nts[i].x * hZoom - hS) + keyW; 
        int sW = (int)((nts[i].x + nts[i].width) * hZoom - hS) + keyW - xPos;
        if (xPos > getWidth() || xPos + sW < keyW) continue;
        auto nR = juce::Rectangle<int>(xPos, yPos + 1, sW - 1, (int)getRowHeight() - 2);

        bool inScale = isNoteInScale(nts[i].pitch);
        juce::Colour noteColor;
        if (selectedNotes.count(i) > 0) { 
            noteColor = juce::Colours::yellow; 
        }
        else if (!inScale) { 
            noteColor = juce::Colour(220, 80, 80); 
        }
        else { 
            noteColor = juce::Colour(130, 90, 190); 
        }

        // Aplicar brillo basado en velocity (0.4 a 1.0 para que no sea negro total)
        float brightness = 0.4f + (nts[i].velocity / 127.0f) * 0.6f;
        noteColor = noteColor.withBrightness(brightness);

        g.setColour(noteColor); g.fillRect(nR);
        g.setColour(juce::Colours::black.withAlpha(0.5f)); g.drawRect(nR, 1.0f);
        g.setColour(juce::Colours::black); g.setFont(juce::Font(std::max(8.0f, getRowHeight() * 0.6f), juce::Font::bold));
        g.drawText(getNoteName(nts[i].pitch), nR, juce::Justification::centred, true);
    }

    for (const auto& anim : animations) {
        int yPos = pitchToY(anim.pitch); 
        int xPos = (int)(anim.x * hZoom - hS) + keyW; 
        int sW = (int)((anim.x + anim.width) * hZoom - hS) + keyW - xPos;
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
        g.fillRect(0, yPos, keyW, (int)getRowHeight());
        g.setColour(wS ? juce::Colours::black : (isB ? juce::Colours::white.withAlpha(0.4f) : juce::Colours::darkgrey));
        g.setFont(juce::Font(std::max(8.0f, getRowHeight() * 0.6f), isWhiteKey(i) ? juce::Font::bold : juce::Font::plain));
        g.drawText(getNoteName(i), 5, yPos, keyW - 10, (int)getRowHeight(), juce::Justification::centredRight);
    }
    // --- ESTÉTICA UNIFICADA: Timeline y Playhead (Estilo Playlist) ---
    g.saveState();
    g.reduceClipRegion(keyW, 0, getWidth() - keyW - vBarWidth, getHeight());
    g.addTransform(juce::AffineTransform::translation((float)keyW, 0.0f));
    
    PlaylistGridRenderer::drawTimelineRuler(g, toolH, 0, timelineH, getWidth() - keyW - vBarWidth, 
                                            hS, hZoom, getContentLengthTicks());

    PlaylistGridRenderer::drawPlayhead(g, (float)playheadAbsPos, hZoom, hS, 
                                       toolH, 0, getWidth() - keyW - vBarWidth, hbY);
    g.restoreState();
    g.setColour(juce::Colours::black.withAlpha(0.9f)); g.fillRect(0, 0, getWidth(), toolH);
}

void PianoRollComponent::resized() {
    snapSelector.setBounds(10, 10, 80, 30);
    toolBtn.setBounds(95, 10, 80, 30);
    pyHumanizeBtn.setBounds(180, 10, 110, 30);
    chordGeneratorBtn.setBounds(295, 10, 130, 30);
    
    linkAutoBtn.setVisible(false); // Mantener desactivado visualmente
    
    rootNoteCombo.setBounds(getWidth() - 320, 10, 50, 30); scaleCombo.setBounds(getWidth() - 260, 10, 240, 30);
    
    int hbY = getHeight() - navigatorH; 
    hBar.setBounds(keyW, hbY, getWidth() - keyW - vBarWidth, navigatorH); 
    vBar.setBounds(getWidth() - vBarWidth, toolH + timelineH, vBarWidth, hbY - (toolH + timelineH));
    
    updateScrollBars(); 
}

void PianoRollComponent::timerCallback() {
    bool needsRepaint = isPlaying;

    if (isPlaying && getPlaybackPosition && !isDraggingPlayhead) {
        float newPos = getPlaybackPosition();
        if (playheadAbsPos != newPos) {
            playheadAbsPos = newPos;
            needsRepaint = true;
        }
    }

    for (int i = (int)animations.size() - 1; i >= 0; --i) { 
        animations[i].alpha -= 0.08f; 
        if (animations[i].alpha <= 0.0f) animations.erase(animations.begin() + i); 
        needsRepaint = true; 
    }
    
    if (needsRepaint) repaint();
}

void PianoRollComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) {
    if (event.mods.isCtrlDown() || event.mods.isCommandDown()) {
        double hS = hBar.getCurrentRangeStart();
        double timeAtMouse = (double)(event.x - keyW + hS) / hZoom;
        double factor = (wheel.deltaY > 0) ? 1.15 : 0.85;
        
        double visibleW = (double)getWidth() - keyW - vBarWidth;
        double effectiveLen = getContentLengthTicks();
        double minHZoom = (double)(visibleW / effectiveLen);
        
        hZoom = juce::jlimit(minHZoom, 10.0, hZoom * factor);
        updateScrollBars();
        hBar.setCurrentRange(timeAtMouse * hZoom - (event.x - keyW), (double)visibleW);
    }
    else if (event.mods.isAltDown()) {
        double currentMouseY = event.y;
        int gridTop = toolH + timelineH;
        if (currentMouseY > gridTop) {
            double vS = vBar.getCurrentRangeStart();
            double mouseAbsY = vS + (currentMouseY - gridTop);
            double zoomFactor = (wheel.deltaY > 0) ? 1.15 : (1.0 / 1.15);
            
            double oldRowHeight = getRowHeight();
            vZoom = juce::jlimit(0.5, 3.0, vZoom * zoomFactor);
            
            double heightRatio = getRowHeight() / oldRowHeight;
            double newVS = mouseAbsY * heightRatio - (currentMouseY - gridTop);
            
            int hbY = getHeight() - navigatorH;
            vBar.setCurrentRange(newVS, (double)(hbY - gridTop));
            updateScrollBars();
        }
    }
    else {
        if (std::abs(wheel.deltaX) > std::abs(wheel.deltaY) || event.mods.isShiftDown()) { 
            double newStart = hBar.getCurrentRangeStart() - (wheel.deltaY * 100.0);
            hBar.setCurrentRange(newStart, (double)(getWidth() - keyW - vBarWidth));
        }
        else {
            double newStart = vBar.getCurrentRangeStart() - (wheel.deltaY * 100.0);
            int hbY = getHeight() - navigatorH;
            vBar.setCurrentRange(newStart, (double)(hbY - (toolH + timelineH)));
        }
    }
    repaint();
}

void PianoRollComponent::mouseMove(const juce::MouseEvent& event) {
    currentMousePos = event.getPosition();
    int hbY = getHeight() - navigatorH; 
    
    int idx = getNoteAt(event.x, event.y); const auto& nts = getNotes();
    double hS_move = hBar.getCurrentRangeStart();
    if (idx != -1 && event.x > (nts[idx].x * hZoom - hS_move + keyW + nts[idx].width * hZoom - 10)) setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else if (idx != -1) setMouseCursor(juce::MouseCursor::DraggingHandCursor); else setMouseCursor(juce::MouseCursor::NormalCursor);
    repaint();
}

void PianoRollComponent::mouseDown(const juce::MouseEvent& event) {
    if (activeClip == nullptr) return;
    isAltDragging = false; hasStateChanged = false; int hbY = getHeight() - navigatorH;
    double hS = hBar.getCurrentRangeStart();
    double absX = std::max(0.0, (double)(event.x - keyW + hS) / hZoom);

    if (event.x < keyW) {
        if (event.y >= toolH + timelineH && event.y < hbY) { previewPitch = yToPitch(event.y); isPreviewingNote = true; }
        return;
    }

    if (event.y >= toolH && event.y < toolH + timelineH) { 
        isDraggingPlayhead = true; 
        playheadAbsPos = (float)(std::round(absX / getSnapPixels()) * getSnapPixels()); 
        repaint(); return; 
    }
    if (event.x > getWidth() - vBarWidth || event.y < toolH + timelineH || event.y >= hbY) return;

    int p = yToPitch(event.y); double f = 440.0 * std::pow(2.0, (p - 69) / 12.0);

    int clickedIdx = getNoteAt(event.x, event.y);
    if (event.mods.isRightButtonDown()) {
        if (clickedIdx != -1) {
            hasStateChanged = true;
            if (selectedNotes.count(clickedIdx)) {
                std::vector<int> toD(selectedNotes.begin(), selectedNotes.end()); std::sort(toD.rbegin(), toD.rend());
                for (int i : toD) { animations.push_back({ activeClip->getNotes()[i].pitch, activeClip->getNotes()[i].x, activeClip->getNotes()[i].width, 1.0f, true }); activeClip->getNotes().erase(activeClip->getNotes().begin() + i); }
            }
            else {
                animations.push_back({ activeClip->getNotes()[clickedIdx].pitch, activeClip->getNotes()[clickedIdx].x, activeClip->getNotes()[clickedIdx].width, 1.0f, true }); activeClip->getNotes().erase(activeClip->getNotes().begin() + clickedIdx);
            }
            selectedNotes.clear(); repaint();
            notifyPatternEdited();
        }
        return;
    }

    if (clickedIdx != -1) {
        if (!selectedNotes.count(clickedIdx)) { if (!event.mods.isShiftDown()) selectedNotes.clear(); selectedNotes.insert(clickedIdx); }
        dragStates.clear(); for (int i : selectedNotes) dragStates.push_back({ i, activeClip->getNotes()[i].x, activeClip->getNotes()[i].pitch, activeClip->getNotes()[i].width });
        int nx = (int)(activeClip->getNotes()[clickedIdx].x * hZoom - hS) + keyW;
        int nw = (int)((activeClip->getNotes()[clickedIdx].x + activeClip->getNotes()[clickedIdx].width) * hZoom - hS) + keyW - nx;
        isResizing = (event.x > (nx + nw - 10));
        dragStartAbsX = (float)absX; dragStartPitch = p; previewPitch = p; isPreviewingNote = true;
    }
    else {
        if (currentTool == ToolMode::Select || event.mods.isCtrlDown() || event.mods.isCommandDown()) { isSelecting = true; selectionStart = event.getPosition(); selectionEnd = event.getPosition(); if (!event.mods.isShiftDown()) selectedNotes.clear(); }
        else {
            hasStateChanged = true; selectedNotes.clear();
            double relX = absX; // Ahora absX ya es relativa al inicio del editor (0-based)
            int fx = (int)std::round(std::floor(relX / getSnapPixels()) * getSnapPixels()); 
            if (fx < 0) fx = 0; 
            if (fx >= 32.0 * 320.0) return;
            activeClip->getNotes().push_back({ p, fx, lastNoteWidth, f }); animations.push_back({ p, fx, lastNoteWidth, 1.0f, false });
            int ni = (int)activeClip->getNotes().size() - 1; selectedNotes.insert(ni); dragStates.clear(); dragStates.push_back({ ni, fx, p, lastNoteWidth }); isResizing = true; dragStartAbsX = (float)absX; dragStartPitch = p; previewPitch = p; isPreviewingNote = true;
            notifyPatternEdited(); // Notificar a la Playlist del nuevo contenido 
        }
    }
    hBar.repaint();
    repaint(); 
}

void PianoRollComponent::mouseDrag(const juce::MouseEvent& event) {
    if (activeClip == nullptr) return;
    currentMousePos = event.getPosition();
    double hS = hBar.getCurrentRangeStart();
    double absX = std::max(0.0, (double)(event.x - keyW + hS) / hZoom);
    if (isDraggingPlayhead) { 
        playheadAbsPos = (float)(std::round(absX / getSnapPixels()) * getSnapPixels()); 
        repaint(); return; 
    }

    int hbY = getHeight() - navigatorH;
    if (event.x < keyW) { if (event.y >= toolH + timelineH && event.y < hbY) { previewPitch = yToPitch(event.y); isPreviewingNote = true; } return; }

    if (isSelecting) { 
        selectionEnd = event.getPosition(); 
        selectionEnd.y = std::max(toolH + timelineH, std::min(getHeight() - vBarWidth, selectionEnd.y)); 
        auto selR = juce::Rectangle<int>(selectionStart, selectionEnd); 
        if (!event.mods.isShiftDown()) selectedNotes.clear(); 
        for (int i = 0; i < (int)activeClip->getNotes().size(); ++i) { 
            int nx = (int)(activeClip->getNotes()[i].x * hZoom - hS) + keyW;
            int nw = (int)((activeClip->getNotes()[i].x + activeClip->getNotes()[i].width) * hZoom - hS) + keyW - nx;
            juce::Rectangle<int> nR(nx, pitchToY(activeClip->getNotes()[i].pitch), nw, (int)getRowHeight()); 
            if (selR.intersects(nR)) selectedNotes.insert(i); 
        } 
        repaint(); return; 
    }
    if (selectedNotes.empty()) return;
    if (event.mods.isAltDown() && !isAltDragging && !isResizing) { hasStateChanged = true; isAltDragging = true; std::set<int> newS; for (int i : selectedNotes) { activeClip->getNotes().push_back(activeClip->getNotes()[i]); newS.insert((int)activeClip->getNotes().size() - 1); } selectedNotes = newS; dragStates.clear(); for (int i : selectedNotes) dragStates.push_back({ i, activeClip->getNotes()[i].x, activeClip->getNotes()[i].pitch, activeClip->getNotes()[i].width }); }
    // Margen elástico seguro: Permitimos mover notas hasta 32 compases (10240 ticks) 
    // Esto evita que choques con el borde pero previene patrones accidentales infinitos
    double maxSafetyLimit = 32.0 * 320.0; 
    
    int snappedDX = (int)(std::round((absX - dragStartAbsX) / getSnapPixels()) * getSnapPixels()); 
    int dP = yToPitch(event.y) - dragStartPitch;
    bool canMP = true; 
    if (!isResizing) {
        for (auto& s : dragStates) {
            if (s.startPitch + dP < 0 || s.startPitch + dP > 127) {
                canMP = false; 
                break;
            }
        }
    }

    for (auto& s : dragStates) {
        if (isResizing) { 
            activeClip->getNotes()[s.index].width = std::max((int)getSnapPixels(), (int)(std::round((s.startWidth + (absX - dragStartAbsX)) / getSnapPixels()) * getSnapPixels())); 
            lastNoteWidth = activeClip->getNotes()[s.index].width; 
        }
        else { 
            int newRelX = s.startX + snappedDX;
            activeClip->getNotes()[s.index].x = juce::jlimit(0, (int)maxSafetyLimit, newRelX); 
            if (canMP && dP != 0) { 
                int newP = s.startPitch + dP; 
                if (activeClip->getNotes()[s.index].pitch != newP) { 
                    activeClip->getNotes()[s.index].pitch = newP; 
                    activeClip->getNotes()[s.index].frequency = 440.0 * std::pow(2.0, (newP - 69) / 12.0); 
                    previewPitch = newP; isPreviewingNote = true; 
                } 
            } 
        }
    }

    notifyPatternEdited();
    hBar.repaint();
    updateScrollBars();
    repaint(); 
}

void PianoRollComponent::mouseUp(const juce::MouseEvent&) {
    isPreviewingNote = false; isDraggingPlayhead = false;
    if (hasStateChanged) { commitHistory(); hasStateChanged = false; }
    if (isSelecting) { isSelecting = false; repaint(); }
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
    // if (key.getKeyCode() == juce::KeyPress::spaceKey) { isPlaying = !isPlaying; repaint(); return true; }
    if (key.getKeyCode() == juce::KeyPress::deleteKey || key.getKeyCode() == juce::KeyPress::backspaceKey) {
        if (!selectedNotes.empty() && activeClip != nullptr) {
            hasStateChanged = true; std::vector<int> toD(selectedNotes.begin(), selectedNotes.end()); std::sort(toD.rbegin(), toD.rend());
            for (int i : toD) { animations.push_back({ activeClip->getNotes()[i].pitch, activeClip->getNotes()[i].x, activeClip->getNotes()[i].width, 1.0f, true }); activeClip->getNotes().erase(activeClip->getNotes().begin() + i); }
            selectedNotes.clear(); commitHistory(); repaint(); 
        } return true;
    }
    return false;
}

void PianoRollComponent::drawMinimap(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (activeClip == nullptr) return;
    
    const auto& notes = activeClip->getNotes();
    if (notes.empty()) return;

    // Rango dinámico para el minimapa
    double maxTime = getContentLengthTicks(); 
    double scaleX = (double)bounds.getWidth() / maxTime;
    double scaleY = (double)bounds.getHeight() / 128.0;

    for (const auto& n : notes)
    {
        float x = (float)(bounds.getX() + n.x * scaleX);
        float w = (float)(n.width * scaleX);
        float y = (float)(bounds.getBottom() - (n.pitch + 1) * scaleY);
        float h = (float)scaleY;

        // Versión simplificada del color original de la nota
        bool inScale = isNoteInScale(n.pitch);
        juce::Colour noteColor = inScale ? juce::Colour(130, 90, 190) : juce::Colour(220, 80, 80);
        g.setColour(noteColor.withAlpha(0.6f));

        g.fillRect(x, y, std::max(1.0f, w), std::max(1.5f, h));
    }
}

double PianoRollComponent::getContentLengthTicks() const
{
    const double barSize = 320.0;
    const double blockSize = 4.0 * barSize; // 1280 ticks (4 Bars)
    double maxNoteEnd = 0.0;
    
    const auto& nts = getNotes();
    for (const auto& n : nts)
    {
        if ((double)n.x + (double)n.width > maxNoteEnd)
            maxNoteEnd = (double)n.x + (double)n.width;
    }
    
    if (maxNoteEnd <= 0.0) return blockSize;
    
    double currentLimit = std::max(blockSize, maxNoteEnd + 1280.0);
    while (maxNoteEnd > (currentLimit - barSize))
    {
        currentLimit += barSize;
    }
    
    return currentLimit;
}