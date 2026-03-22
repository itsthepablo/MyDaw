#include "PlaylistComponent.h"
#include "../UI/WaveformRenderer.h" 
#include <cmath>
#include <algorithm>

PlaylistComponent::PlaylistComponent() {
    setWantsKeyboardFocus(true);
    addKeyListener(this);

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
    if (audioMutex) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

    if (tc.linkedAudio) tc.trackPtr->audioClips.removeObject(tc.linkedAudio, true);
    if (tc.linkedMidi) tc.trackPtr->midiClips.removeObject(tc.linkedMidi, true);

    clips.erase(clips.begin() + index);

    if (selectedClipIndex == index) selectedClipIndex = -1;
    else if (selectedClipIndex > index) selectedClipIndex--;

    repaint();
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

    for (double i = 0; i <= 32 * 320; i += 80.0) {
        int dx = (int)(i * hZoom) - (int)hS;
        if (dx < 0) continue;
        g.setColour(juce::Colours::white.withAlpha(0.05f));
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
            g.saveState();
            g.reduceClipRegion(clipRect);

            // --- ESTILO VISUAL DE EDICIÓN INLINE ---
            if (clip.trackPtr->isInlineEditingActive) {
                // Pintamos un fondo más brillante para indicar que está "Abierto"
                g.setColour(juce::Colours::white.withAlpha(0.1f));
                g.fillRoundedRectangle(clipRect.toFloat(), 4.0f);

                // Grilla de cuantización interior
                g.setColour(juce::Colours::white.withAlpha(0.05f));
                for (float gridX = clip.startX; gridX < clip.startX + clip.width; gridX += 20.0f) {
                    int sx = xPos + (int)((gridX - clip.startX) * hZoom);
                    g.drawVerticalLine(sx, (float)clipRect.getY(), (float)clipRect.getBottom());
                }
            }

            if (!clip.linkedMidi->notes.empty()) {
                int minPitch = 127; int maxPitch = 0;
                for (const auto& note : clip.linkedMidi->notes) {
                    if (note.pitch < minPitch) minPitch = note.pitch;
                    if (note.pitch > maxPitch) maxPitch = note.pitch;
                }

                minPitch = std::max(0, minPitch - 3);
                maxPitch = std::min(127, maxPitch + 3);
                int pitchRange = std::max(12, maxPitch - minPitch);

                for (const auto& note : clip.linkedMidi->notes) {
                    float normalizedY = 1.0f - ((float)(note.pitch - minPitch) / (float)pitchRange);
                    float noteY = clipRect.getY() + 4.0f + (normalizedY * (clipRect.getHeight() - 12.0f));

                    int noteScreenX = (int)(note.x * hZoom) - (int)hS;
                    int noteScreenW = std::max(3, (int)(note.width * hZoom));

                    juce::Rectangle<float> miniNoteRect((float)noteScreenX, noteY, (float)noteScreenW, 5.0f);

                    g.setColour(clip.trackPtr->getColor().brighter(0.5f));
                    g.fillRoundedRectangle(miniNoteRect, 1.0f);

                    g.setColour(juce::Colours::black.withAlpha(0.8f));
                    g.drawRoundedRectangle(miniNoteRect, 1.0f, 1.0f);
                }
            }
            else {
                g.setColour(clip.trackPtr->getColor().withAlpha(0.3f));
                for (int j = 6; j < clipRect.getHeight(); j += 10) {
                    g.drawHorizontalLine(clipRect.getY() + j, (float)clipRect.getX() + 2.0f, (float)clipRect.getRight() - 2.0f);
                }
            }
            g.restoreState();
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
        float snappedX = std::round(absX / 80.0f) * 80.0f;

        auto* newMidiClip = new MidiClipData();
        newMidiClip->name = "Pattern " + juce::String((*tracksRef)[tIdx]->midiClips.size() + 1);
        newMidiClip->startX = snappedX;
        newMidiClip->width = 320.0f;
        newMidiClip->color = (*tracksRef)[tIdx]->getColor();

        (*tracksRef)[tIdx]->midiClips.add(newMidiClip);
        clips.push_back({ (*tracksRef)[tIdx], snappedX, 320.0f, newMidiClip->name, nullptr, newMidiClip });
        repaint();
    }
}

void PlaylistComponent::mouseDown(const juce::MouseEvent& e) {
    grabKeyboardFocus();

    int cIdx = getClipAt(e.x, e.y);
    selectedClipIndex = cIdx;

    if (cIdx != -1) {
        if (e.mods.isRightButtonDown()) {
            juce::PopupMenu m;
            m.addItem(1, "Eliminar Clip");
            m.showMenuAsync(juce::PopupMenu::Options(), [this, cIdx](int result) {
                if (result == 1) deleteClip(cIdx);
                });
            return;
        }

        auto& clip = clips[cIdx];
        float hS = (float)hBar.getCurrentRangeStart();

        // --- LÓGICA HIT-TESTING DE EDICIÓN INLINE ---
        if (clip.trackPtr->isInlineEditingActive && clip.linkedMidi && !clip.linkedMidi->notes.empty()) {
            int yPos = getTrackY(clip.trackPtr);
            int xPos = (int)(clip.startX * hZoom) - (int)hS;
            int wPos = (int)(clip.width * hZoom);
            juce::Rectangle<int> clipRect(xPos, yPos + 2, wPos - 1, (int)trackHeight - 4);

            int minPitch = 127, maxPitch = 0;
            for (const auto& n : clip.linkedMidi->notes) {
                if (n.pitch < minPitch) minPitch = n.pitch;
                if (n.pitch > maxPitch) maxPitch = n.pitch;
            }
            minPitch = std::max(0, minPitch - 3);
            maxPitch = std::min(127, maxPitch + 3);
            int pitchRange = std::max(12, maxPitch - minPitch);

            for (int i = (int)clip.linkedMidi->notes.size() - 1; i >= 0; --i) {
                auto& note = clip.linkedMidi->notes[i];
                float normalizedY = 1.0f - ((float)(note.pitch - minPitch) / (float)pitchRange);
                float noteY = clipRect.getY() + 4.0f + (normalizedY * (clipRect.getHeight() - 12.0f));
                int noteScreenX = (int)(note.x * hZoom) - (int)hS;
                int noteScreenW = std::max(3, (int)(note.width * hZoom));

                juce::Rectangle<float> noteRect((float)noteScreenX, noteY, (float)noteScreenW, 5.0f);

                // Expandimos el área de clic (Hitbox) para que sea fácil agarrar las notas pequeñas
                if (noteRect.expanded(2.0f, 4.0f).contains((float)e.x, (float)e.y)) {
                    draggingClipIndex = cIdx;
                    draggingNoteIndex = i;
                    dragStartAbsX = (e.x + hS) / hZoom;
                    dragStartNoteX = note.x;
                    dragStartNoteWidth = note.width;
                    isResizingNote = e.x > (noteScreenX + noteScreenW - 6);
                    repaint();
                    return; // Encontramos la nota, detenemos la búsqueda
                }
            }
        }

        // Si no hicimos clic en una nota, agarramos el clip completo (Flujo normal)
        draggingClipIndex = cIdx;
        draggingNoteIndex = -1; // Nos aseguramos de desmarcar las notas
        dragStartAbsX = (e.x + hS) / hZoom;
        dragStartXOriginal = clips[cIdx].startX;
        dragStartWidth = clips[cIdx].width;
        isResizingClip = e.x > ((clips[cIdx].startX * hZoom - hS) + clips[cIdx].width * hZoom - 10);
    }
    else {
        draggingClipIndex = -1;
        draggingNoteIndex = -1;
    }
    repaint();
}

void PlaylistComponent::mouseDrag(const juce::MouseEvent& e) {
    if (draggingClipIndex == -1) return;

    float absX = (e.x + (float)hBar.getCurrentRangeStart()) / hZoom;
    float diff = absX - dragStartAbsX;

    // --- ARRASTRE DE NOTAS INLINE (Tiempo y Resize) ---
    if (draggingNoteIndex != -1) {
        auto* midiClip = clips[draggingClipIndex].linkedMidi;
        auto& note = midiClip->notes[draggingNoteIndex];

        // Cuantización de notas a 1/16 de beat (20 píxeles) para una edición musical precisa
        float snappedX = std::round((dragStartNoteX + diff) / 20.0f) * 20.0f;

        if (isResizingNote) {
            note.width = juce::jmax(10.0f, dragStartNoteWidth + diff);
        }
        else {
            // Restringimos la nota para que no pueda salirse por la izquierda del clip
            note.x = juce::jmax(midiClip->startX, snappedX);
        }
        repaint();
        return; // Salimos para no mover el clip entero
    }

    // --- ARRASTRE NORMAL DE CLIPS (Vertical y Horizontal) ---
    int newTrackIdx = getTrackAtY(e.y);
    if (newTrackIdx != -1 && (*tracksRef)[newTrackIdx] != clips[draggingClipIndex].trackPtr && !isResizingClip) {
        Track* newTrack = (*tracksRef)[newTrackIdx];
        Track* oldTrack = clips[draggingClipIndex].trackPtr;

        bool isAudio = clips[draggingClipIndex].linkedAudio != nullptr;
        bool isMidi = clips[draggingClipIndex].linkedMidi != nullptr;

        if ((isAudio && newTrack->getType() == TrackType::Audio) || (isMidi && newTrack->getType() == TrackType::MIDI)) {
            std::unique_ptr<juce::ScopedLock> lock;
            if (audioMutex) lock = std::make_unique<juce::ScopedLock>(*audioMutex);

            if (isMidi) {
                auto* m = clips[draggingClipIndex].linkedMidi;
                oldTrack->midiClips.removeObject(m, false);
                newTrack->midiClips.add(m);
            }
            else if (isAudio) {
                auto* a = clips[draggingClipIndex].linkedAudio;
                oldTrack->audioClips.removeObject(a, false);
                newTrack->audioClips.add(a);
            }
            clips[draggingClipIndex].trackPtr = newTrack;
        }
    }

    float snappedX = std::round((dragStartXOriginal + diff) / 80.0f) * 80.0f;
    snappedX = juce::jmax(0.0f, snappedX);

    if (isResizingClip) {
        float newW = juce::jmax(10.0f, dragStartWidth + diff);
        clips[draggingClipIndex].width = newW;
        if (clips[draggingClipIndex].linkedAudio != nullptr) clips[draggingClipIndex].linkedAudio->width = newW;
        if (clips[draggingClipIndex].linkedMidi != nullptr) clips[draggingClipIndex].linkedMidi->width = newW;
    }
    else {
        if (clips[draggingClipIndex].linkedMidi != nullptr) {
            float timeShift = snappedX - clips[draggingClipIndex].linkedMidi->startX;
            for (auto& note : clips[draggingClipIndex].linkedMidi->notes) {
                note.x += timeShift;
            }
        }

        clips[draggingClipIndex].startX = snappedX;
        if (clips[draggingClipIndex].linkedAudio != nullptr) clips[draggingClipIndex].linkedAudio->startX = snappedX;
        if (clips[draggingClipIndex].linkedMidi != nullptr) clips[draggingClipIndex].linkedMidi->startX = snappedX;
    }
    repaint();
}

void PlaylistComponent::mouseUp(const juce::MouseEvent&) {
    draggingClipIndex = -1;
    draggingNoteIndex = -1;
}

void PlaylistComponent::mouseMove(const juce::MouseEvent& e) {
    int idx = getClipAt(e.x, e.y);
    if (idx != -1) {
        auto& clip = clips[idx];
        float hS = (float)hBar.getCurrentRangeStart();

        // --- CAMBIO DE CURSOR PARA NOTAS INLINE ---
        if (clip.trackPtr->isInlineEditingActive && clip.linkedMidi && !clip.linkedMidi->notes.empty()) {
            int yPos = getTrackY(clip.trackPtr);
            int xPos = (int)(clip.startX * hZoom) - (int)hS;
            int wPos = (int)(clip.width * hZoom);
            juce::Rectangle<int> clipRect(xPos, yPos + 2, wPos - 1, (int)trackHeight - 4);

            int minPitch = 127, maxPitch = 0;
            for (const auto& n : clip.linkedMidi->notes) {
                if (n.pitch < minPitch) minPitch = n.pitch;
                if (n.pitch > maxPitch) maxPitch = n.pitch;
            }
            minPitch = std::max(0, minPitch - 3);
            maxPitch = std::min(127, maxPitch + 3);
            int pitchRange = std::max(12, maxPitch - minPitch);

            for (const auto& note : clip.linkedMidi->notes) {
                float normalizedY = 1.0f - ((float)(note.pitch - minPitch) / (float)pitchRange);
                float noteY = clipRect.getY() + 4.0f + (normalizedY * (clipRect.getHeight() - 12.0f));
                int noteScreenX = (int)(note.x * hZoom) - (int)hS;
                int noteScreenW = std::max(3, (int)(note.width * hZoom));

                juce::Rectangle<float> noteRect((float)noteScreenX, noteY, (float)noteScreenW, 5.0f);

                if (noteRect.expanded(2.0f, 4.0f).contains((float)e.x, (float)e.y)) {
                    if (e.x > noteScreenX + noteScreenW - 6) setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
                    else setMouseCursor(juce::MouseCursor::DraggingHandCursor);
                    return;
                }
            }
        }

        // --- CAMBIO DE CURSOR PARA CLIP NORMAL ---
        float edgeX = (clip.startX * hZoom - hS) + clip.width * hZoom;
        if (e.x > edgeX - 10) setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        else setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    else {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
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
    absX = std::round(absX / 80.0f) * 80.0f;
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