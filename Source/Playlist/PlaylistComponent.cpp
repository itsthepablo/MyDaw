#include "PlaylistComponent.h"
#include "../UI/WaveformRenderer.h" // REQUERIDO: Motor de dibujo pro
#include <cmath>
#include <algorithm>

PlaylistComponent::PlaylistComponent() {
    addAndMakeVisible(hBar); hBar.addListener(this);
    addAndMakeVisible(vBar); vBar.addListener(this);
    vBar.setAlwaysOnTop(true);
    updateScrollBars();
    startTimerHz(30);
}

PlaylistComponent::~PlaylistComponent() { stopTimer(); }

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

void PlaylistComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(25, 27, 30));
    float hS = (float)hBar.getCurrentRangeStart();
    float vS = (float)vBar.getCurrentRangeStart();
    int viewAreaY = timelineH;
    int viewAreaH = getHeight() - timelineH - scrollBarSize;

    g.saveState();
    g.reduceClipRegion(0, viewAreaY, getWidth() - (vBar.isVisible() ? scrollBarSize : 0), viewAreaH);

    // --- GRILLA (Grid visible detrás) ---
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

    // --- DIBUJO DE CLIPS CON WAVEFORM CACHÉ ---
    for (const auto& clip : clips) {
        int yPos = getTrackY(clip.trackPtr);
        if (yPos < timelineH - 100 || yPos > getHeight()) continue;
        int xPos = (int)(clip.startX * hZoom) - (int)hS;
        int wPos = (int)(clip.width * hZoom);
        juce::Rectangle<int> clipRect(xPos, yPos + 2, wPos - 1, (int)trackHeight - 4);

        // 1. Fondo Clip: Transparente (20%) para ver la grilla
        g.setColour(clip.trackPtr->getColor().withAlpha(0.2f));
        g.fillRoundedRectangle(clipRect.toFloat(), 4.0f);

        // 2. Renderizado Waveform instantáneo desde el caché
        if (clip.linkedAudio != nullptr) {
            WaveformRenderer::drawWaveform(g, *clip.linkedAudio, clipRect, clip.trackPtr->getColor());
        }

        // 3. Texto
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.drawText(clip.name, clipRect.reduced(5, 2), juce::Justification::topLeft, true);
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

void PlaylistComponent::mouseDown(const juce::MouseEvent& e) {
    int cIdx = getClipAt(e.x, e.y);
    if (cIdx != -1) {
        draggingClipIndex = cIdx;
        dragStartAbsX = (e.x + (float)hBar.getCurrentRangeStart()) / hZoom;
        dragStartXOriginal = clips[cIdx].startX;
        dragStartWidth = clips[cIdx].width;
        isResizingClip = e.x > ((clips[cIdx].startX * hZoom - hBar.getCurrentRangeStart()) + clips[cIdx].width * hZoom - 10);
    }
}

void PlaylistComponent::mouseDrag(const juce::MouseEvent& e) {
    if (draggingClipIndex == -1) return;
    float absX = (e.x + (float)hBar.getCurrentRangeStart()) / hZoom;
    float diff = absX - dragStartAbsX;
    if (isResizingClip) clips[draggingClipIndex].width = juce::jmax(10.0f, dragStartWidth + diff);
    else clips[draggingClipIndex].startX = juce::jmax(0.0f, dragStartXOriginal + diff);
    repaint();
}

void PlaylistComponent::mouseUp(const juce::MouseEvent&) { draggingClipIndex = -1; }

void PlaylistComponent::mouseMove(const juce::MouseEvent& e) {
    int idx = getClipAt(e.x, e.y);
    if (idx != -1) {
        float edgeX = (clips[idx].startX * hZoom - hBar.getCurrentRangeStart()) + clips[idx].width * hZoom;
        if (e.x > edgeX - 10) setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        else setMouseCursor(juce::MouseCursor::DraggingHandCursor);
    }
    else setMouseCursor(juce::MouseCursor::NormalCursor);
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

            // --- LLAMADA CRÍTICA: Generamos el caché de dibujo una sola vez al cargar ---
            data->generateCache();

            data->width = (float)(reader->lengthInSamples / reader->sampleRate) * (120.0f / 60.0f) * 80.0f;
            (*tracksRef)[tIdx]->audioClips.add(data);
            clips.push_back({ (*tracksRef)[tIdx], absX, data->width, data->name, data });
            absX += data->width;
        }
    }
    updateScrollBars(); repaint();
}