#include "PlaylistComponent.h"
#include "../UI/WaveformRenderer.h" 
#include "../UI/MidiClipRenderer.h"
#include "Tools/PointerTool.h" 
#include "Tools/ScissorTool.h" 
#include "Tools/EraserTool.h" 
#include <cmath>
#include <algorithm>

PlaylistComponent::PlaylistComponent() {
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    activeTool = std::make_unique<PointerTool>();

    addAndMakeVisible(hBar); hBar.addListener(this);
    addAndMakeVisible(vBar); vBar.addListener(this);
    vBar.setAlwaysOnTop(true);
    updateScrollBars();
    startTimerHz(30);
}

PlaylistComponent::~PlaylistComponent() {
    removeKeyListener(this);
    stopTimer();
}

void PlaylistComponent::timerCallback() { repaint(); }

void PlaylistComponent::updateScrollBars() {
    hBar.setRangeLimits(0.0, 32 * 320 * hZoom);
    hBar.setCurrentRange(hBar.getCurrentRangeStart(), (double)getWidth() - scrollBarSize);
    int totalH = 0;
    if (tracksRef) {
        for (auto* t : *tracksRef) if (t->isShowingInChildren) totalH += (int)trackHeight;
    }
    double visibleH = (double)getHeight() - timelineH - scrollBarSize;
    vBar.setRangeLimits(0.0, (double)totalH);
    vBar.setCurrentRange(vBar.getCurrentRangeStart(), visibleH);
    vBar.setVisible(totalH > visibleH);
}

void PlaylistComponent::addMidiClipToView(Track* targetTrack, MidiClipData* newClip) {
    clips.push_back({ targetTrack, newClip->startX, newClip->width, newClip->name, nullptr, newClip });
    updateScrollBars();
    repaint();
}

int PlaylistComponent::getTrackY(Track* targetTrack) const {
    if (!tracksRef) return -1;
    int currentY = timelineH - (int)vBar.getCurrentRangeStart();
    for (auto* t : *tracksRef) {
        if (t == targetTrack) return currentY;
        if (t->isShowingInChildren) currentY += (int)trackHeight;
    }
    return -1;
}

int PlaylistComponent::getTrackAtY(int y) const {
    if (y < timelineH) return -1;
    int vS = (int)vBar.getCurrentRangeStart();
    int currentY = timelineH - vS;
    if (!tracksRef) return -1;
    for (int i = 0; i < (int)tracksRef->size(); ++i) {
        auto* t = (*tracksRef)[i];
        if (!t->isShowingInChildren) continue;
        if (y >= currentY && y < currentY + trackHeight) return i;
        currentY += (int)trackHeight;
    }
    return -1;
}

void PlaylistComponent::deleteClip(int index) {
    if (index < 0 || index >= (int)clips.size()) return;

    auto& tc = clips[index];

    if (tc.linkedMidi && onMidiClipDeleted) {
        onMidiClipDeleted(tc.linkedMidi);
    }

    std::unique_ptr<juce::ScopedLock> lock;
    if (audioMutex != nullptr) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

    if (tc.linkedAudio) tc.trackPtr->audioClips.removeObject(tc.linkedAudio, true);
    if (tc.linkedMidi) tc.trackPtr->midiClips.removeObject(tc.linkedMidi, true);

    clips.erase(clips.begin() + index);

    if (selectedClipIndex == index) selectedClipIndex = -1;
    else if (selectedClipIndex > index) selectedClipIndex--;

    repaint();
}

void PlaylistComponent::setTool(int toolId) {
    if (toolId == 1) activeTool = std::make_unique<PointerTool>();
    else if (toolId == 3) activeTool = std::make_unique<ScissorTool>();
    else if (toolId == 4) activeTool = std::make_unique<EraserTool>();
}

bool PlaylistComponent::keyPressed(const juce::KeyPress& key, juce::Component*) {
    if (key.getKeyCode() == juce::KeyPress::deleteKey || key.getKeyCode() == juce::KeyPress::backspaceKey) {
        if (selectedClipIndex != -1) {
            deleteClip(selectedClipIndex);
            return true;
        }
    }
    return false;
}

void PlaylistComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(25, 27, 30));
    float hS = (float)hBar.getCurrentRangeStart();
    float vS = (float)vBar.getCurrentRangeStart();
    int viewAreaY = timelineH;
    int viewAreaH = getHeight() - timelineH - scrollBarSize;

    g.saveState();
    g.reduceClipRegion(0, viewAreaY, getWidth() - (vBar.isVisible() ? scrollBarSize : 0), viewAreaH);

    double visualSnap = (snapPixels < 10.0) ? 80.0 : snapPixels;

    for (double i = 0; i <= 32 * 320; i += visualSnap) {
        int dx = (int)(i * hZoom) - (int)hS;
        if (dx < 0) continue;

        if (std::fmod(i, 80.0) == 0.0) {
            g.setColour(juce::Colours::white.withAlpha(0.1f));
        }
        else {
            g.setColour(juce::Colours::white.withAlpha(0.04f));
        }

        g.drawVerticalLine(dx, (float)viewAreaY, (float)getHeight());
    }

    int currentY = timelineH - (int)vS;
    if (tracksRef) {
        for (auto* t : *tracksRef) {
            if (!t->isShowingInChildren) continue;
            g.setColour(juce::Colours::black.withAlpha(0.4f));
            g.drawHorizontalLine(currentY + (int)trackHeight - 1, 0.0f, (float)getWidth());
            currentY += (int)trackHeight;
        }
    }

    for (int i = 0; i < (int)clips.size(); ++i) {
        const auto& clip = clips[i];
        int yPos = getTrackY(clip.trackPtr);
        if (yPos < timelineH - 100 || yPos > getHeight()) continue;

        int xPos = (int)(clip.startX * hZoom) - (int)hS;
        int wPos = (int)(clip.width * hZoom);
        juce::Rectangle<int> clipRect(xPos, yPos + 2, wPos - 1, (int)trackHeight - 4);

        g.setColour(clip.trackPtr->getColor().withAlpha(0.2f));
        g.fillRoundedRectangle(clipRect.toFloat(), 4.0f);

        if (clip.linkedAudio != nullptr) {
            WaveformRenderer::drawWaveform(g, *clip.linkedAudio, clipRect, clip.trackPtr->getColor(), clip.trackPtr->getWaveformViewMode());
        }

        if (clip.linkedMidi != nullptr) {
            MidiClipRenderer::drawMidiClip(g, *clip.linkedMidi, clipRect, clip.trackPtr->getColor(), clip.trackPtr->isInlineEditingActive, hZoom, hS);
        }

        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawText(clip.name, clipRect.reduced(5, 2), juce::Justification::topLeft, true);

        if (i == selectedClipIndex) {
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(clipRect.toFloat(), 4.0f, 1.5f);
        }
    }
    g.restoreState();

    g.setColour(juce::Colour(20, 22, 25)); g.fillRect(0, 0, getWidth(), timelineH);
    int phX = (int)(playheadAbsPos * hZoom) - (int)hS;
    g.setColour(juce::Colours::red); g.drawVerticalLine(phX, 0, (float)getHeight());

    if (isExternalFileDragging) {
        g.fillAll(juce::Colours::dodgerblue.withAlpha(0.2f));
        g.setColour(juce::Colours::white);
        g.drawText("SUELTA TU AUDIO AQUI", getLocalBounds(), juce::Justification::centred);
    }
}

void PlaylistComponent::resized() {
    hBar.setBounds(0, getHeight() - scrollBarSize, getWidth() - scrollBarSize, scrollBarSize);
    vBar.setBounds(getWidth() - scrollBarSize, timelineH, scrollBarSize, getHeight() - timelineH - scrollBarSize);
    updateScrollBars();
}

void PlaylistComponent::scrollBarMoved(juce::ScrollBar* s, double start) {
    if (s == &vBar && onVerticalScroll) onVerticalScroll((int)start);
    repaint();
}

void PlaylistComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) {
    if (e.mods.isCtrlDown()) {
        hZoom = juce::jlimit(0.1f, 5.0f, hZoom + (float)w.deltaY * 0.1f);
        updateScrollBars();
    }
    else vBar.mouseWheelMove(e, w);
    repaint();
}

int PlaylistComponent::getClipAt(int x, int y) const {
    int tIdx = getTrackAtY(y); if (tIdx == -1) return -1;
    float absX = (x + (float)hBar.getCurrentRangeStart()) / hZoom;
    for (int i = (int)clips.size() - 1; i >= 0; --i) {
        if (clips[i].trackPtr == (*tracksRef)[tIdx] && absX >= clips[i].startX && absX <= clips[i].startX + clips[i].width) return i;
    }
    return -1;
}

// --- MODIFICADO: DOBLE CLIC INTELIGENTE (COPIA PROFUNDA) ---
void PlaylistComponent::mouseDoubleClick(const juce::MouseEvent& e) {
    if (!tracksRef) return;
    int cIdx = getClipAt(e.x, e.y);

    if (cIdx != -1 && clips[cIdx].linkedMidi != nullptr) {
        if (onMidiClipDoubleClicked) {
            onMidiClipDoubleClicked(clips[cIdx].trackPtr, clips[cIdx].linkedMidi);
        }
        return;
    }

    int tIdx = getTrackAtY(e.y);
    if (tIdx != -1 && (*tracksRef)[tIdx]->getType() == TrackType::MIDI && cIdx == -1) {
        float absX = (e.x + (float)hBar.getCurrentRangeStart()) / hZoom;
        float snappedX = std::round(absX / snapPixels) * snapPixels;

        Track* targetTrack = (*tracksRef)[tIdx];
        MidiClipData* newMidiClip = nullptr;

        // ¿EL TRACK YA TIENE UN PATRÓN PREVIO?
        if (!targetTrack->midiClips.isEmpty()) {
            // SÍ: Clonamos el último patrón editado (Copia Profunda para evitar colisión de coordenadas)
            MidiClipData* sourceClip = targetTrack->midiClips.getLast();
            newMidiClip = new MidiClipData(*sourceClip);

            float timeShift = snappedX - sourceClip->startX;
            newMidiClip->startX = snappedX;

            // Desplazamos las coordenadas absolutas de las notas para que coincidan con la nueva posición
            for (auto& note : newMidiClip->notes) {
                note.x += timeShift;
            }

            // Desplazamos las coordenadas absolutas de los nodos de automatización
            auto shiftAutoLane = [timeShift](AutoLane& lane) {
                for (auto& node : lane.nodes) {
                    node.x += timeShift;
                }
                };
            shiftAutoLane(newMidiClip->autoVol);
            shiftAutoLane(newMidiClip->autoPan);
            shiftAutoLane(newMidiClip->autoPitch);
            shiftAutoLane(newMidiClip->autoFilter);

            // Nota: El nombre se mantiene igual (ej. "Pattern 1"), por lo que el Picker no lo duplicará.
        }
        else {
            // NO: El track está vacío. Creamos un patrón completamente nuevo
            int maxPatternNum = 0;
            for (auto* tr : *tracksRef) {
                for (auto* mc : tr->midiClips) {
                    if (mc->name.startsWith("Pattern ")) {
                        int num = mc->name.substring(8).getIntValue();
                        if (num > maxPatternNum) maxPatternNum = num;
                    }
                }
            }
            int nextPatternNum = maxPatternNum + 1;

            newMidiClip = new MidiClipData();
            newMidiClip->name = "Pattern " + juce::String(nextPatternNum);
            newMidiClip->startX = snappedX;
            newMidiClip->width = 320.0f;
            newMidiClip->color = targetTrack->getColor();
        }

        targetTrack->midiClips.add(newMidiClip);
        clips.push_back({ targetTrack, snappedX, newMidiClip->width, newMidiClip->name, nullptr, newMidiClip });
        repaint();
    }
}

void PlaylistComponent::mouseDown(const juce::MouseEvent& e) {
    if (activeTool) activeTool->mouseDown(e, *this);
}

void PlaylistComponent::mouseDrag(const juce::MouseEvent& e) {
    if (activeTool) activeTool->mouseDrag(e, *this);
}

void PlaylistComponent::mouseUp(const juce::MouseEvent& e) {
    if (activeTool) activeTool->mouseUp(e, *this);
}

void PlaylistComponent::mouseMove(const juce::MouseEvent& e) {
    if (activeTool) activeTool->mouseMove(e, *this);
}

bool PlaylistComponent::isInterestedInFileDrag(const juce::StringArray&) { return true; }
void PlaylistComponent::fileDragEnter(const juce::StringArray&, int, int) { isExternalFileDragging = true; repaint(); }
void PlaylistComponent::fileDragExit(const juce::StringArray&) { isExternalFileDragging = false; repaint(); }

void PlaylistComponent::filesDropped(const juce::StringArray& files, int x, int y) {
    isExternalFileDragging = false;
    int tIdx = getTrackAtY(y);
    if (tIdx == -1 && onNewTrackRequested) {
        onNewTrackRequested();
        if (tracksRef) tIdx = (int)tracksRef->size() - 1;
    }
    if (tIdx == -1) return;

    float absX = (x + (float)hBar.getCurrentRangeStart()) / hZoom;

    absX = std::round(absX / snapPixels) * snapPixels;
    absX = juce::jmax(0.0f, absX);

    juce::AudioFormatManager fmt; fmt.registerBasicFormats();
    for (auto path : files) {
        juce::File f(path);
        std::unique_ptr<juce::AudioFormatReader> reader(fmt.createReaderFor(f));
        if (reader) {
            auto* data = new AudioClipData();
            data->name = f.getFileNameWithoutExtension();
            data->startX = absX;
            data->fileBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
            reader->read(&data->fileBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

            data->generateCache();

            data->width = (float)(reader->lengthInSamples / reader->sampleRate) * (120.0f / 60.0f) * 80.0f;
            (*tracksRef)[tIdx]->audioClips.add(data);
            clips.push_back({ (*tracksRef)[tIdx], absX, data->width, data->name, data, nullptr });
            absX += data->width;
        }
    }
    updateScrollBars(); repaint();
}