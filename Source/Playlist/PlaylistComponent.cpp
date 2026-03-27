#include "PlaylistComponent.h"
#include "../UI/WaveformRenderer.h" 
#include "../UI/MidiClipRenderer.h"
#include "Tools/PointerTool.h" 
#include "Tools/ScissorTool.h" 
#include "Tools/EraserTool.h" 
#include "PlaylistDropHandler.h"
#include "PlaylistActionHandler.h" // <-- NUESTRO NUEVO MANEJADOR DE LOGICA PESADA
#include <cmath>
#include <algorithm>

PlaylistComponent::PlaylistComponent() {
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    activeTool = std::make_unique<PointerTool>();

    addAndMakeVisible(menuBar);
    addAndMakeVisible(hNavigator);
    
    hNavigator.onScrollMoved = [this](double) { repaint(); };

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
    hNavigator.setRangeLimits(0.0, 32 * 320 * hZoom);
    hNavigator.setCurrentRange(hNavigator.getCurrentRangeStart(), (double)getWidth() - scrollBarSize);
    
    int totalH = 0;
    if (tracksRef) {
        for (auto* t : *tracksRef) if (t->isShowingInChildren) totalH += (int)trackHeight;
    }
    double visibleH = (double)getHeight() - menuBarH - timelineH - scrollBarSize;
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
    int currentY = menuBarH + timelineH - (int)vBar.getCurrentRangeStart();
    for (auto* t : *tracksRef) {
        if (t == targetTrack) return currentY;
        if (t->isShowingInChildren) currentY += (int)trackHeight;
    }
    return -1;
}

int PlaylistComponent::getTrackAtY(int y) const {
    if (y < menuBarH + timelineH) return -1;
    int vS = (int)vBar.getCurrentRangeStart();
    int currentY = menuBarH + timelineH - vS;
    if (!tracksRef) return -1;
    for (int i = 0; i < (int)tracksRef->size(); ++i) {
        auto* t = (*tracksRef)[i];
        if (!t->isShowingInChildren) continue;
        if (y >= currentY && y < currentY + trackHeight) return i;
        currentY += (int)trackHeight;
    }
    return -1;
}

// --- DELEGACIÓN DE ACCIONES ---

void PlaylistComponent::deleteClip(int index) {
    PlaylistActionHandler::deleteClip(*this, index);
}

void PlaylistComponent::deleteClipsByName(const juce::String& name, bool isMidi) {
    PlaylistActionHandler::deleteClipsByName(*this, name, isMidi);
}

void PlaylistComponent::purgeClipsOfTrack(Track* track) {
    PlaylistActionHandler::purgeClipsOfTrack(*this, track);
}

void PlaylistComponent::mouseDoubleClick(const juce::MouseEvent& e) {
    PlaylistActionHandler::handleDoubleClick(*this, e);
}

// --- HERRAMIENTAS Y TECLADO ---

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

// --- RENDERIZADO VISUAL ---

void PlaylistComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(25, 27, 30));
    float hS = (float)hNavigator.getCurrentRangeStart();
    float vS = (float)vBar.getCurrentRangeStart();
    int viewAreaY = menuBarH + timelineH;
    int viewAreaH = getHeight() - menuBarH - timelineH - scrollBarSize;

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

    int currentY = menuBarH + timelineH - (int)vS;
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
        if (yPos < menuBarH + timelineH - 100 || yPos > getHeight()) continue;

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

    g.setColour(juce::Colour(20, 22, 25)); g.fillRect(0, menuBarH, getWidth(), timelineH);
    int phX = (int)(playheadAbsPos * hZoom) - (int)hS;
    g.setColour(juce::Colours::red); g.drawVerticalLine(phX, (float)menuBarH, (float)getHeight());

    if (isExternalFileDragging || isInternalDragging) {
        g.fillAll(juce::Colours::dodgerblue.withAlpha(0.2f));
        g.setColour(juce::Colours::white);
        juce::String overlayText = isInternalDragging ? "SUELTA EL CLIP AQUI" : "SUELTA TU AUDIO AQUI";
        g.drawText(overlayText, getLocalBounds(), juce::Justification::centred);
    }
}

void PlaylistComponent::resized() {
    menuBar.setBounds(0, 0, getWidth(), menuBarH);
    hNavigator.setBounds(0, getHeight() - scrollBarSize, getWidth() - scrollBarSize, scrollBarSize);
    vBar.setBounds(getWidth() - scrollBarSize, menuBarH + timelineH, scrollBarSize, getHeight() - menuBarH - timelineH - scrollBarSize);
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
    float absX = (x + (float)hNavigator.getCurrentRangeStart()) / hZoom;
    for (int i = (int)clips.size() - 1; i >= 0; --i) {
        if (clips[i].trackPtr == (*tracksRef)[tIdx] && absX >= clips[i].startX && absX <= clips[i].startX + clips[i].width) return i;
    }
    return -1;
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

// --- ARRASTRAR Y SOLTAR ARCHIVOS DELEGADOS ---

bool PlaylistComponent::isInterestedInFileDrag(const juce::StringArray&) { return true; }

void PlaylistComponent::fileDragEnter(const juce::StringArray&, int, int) { 
    isExternalFileDragging = true; 
    repaint(); 
}

void PlaylistComponent::fileDragExit(const juce::StringArray&) { 
    isExternalFileDragging = false; 
    repaint(); 
}

void PlaylistComponent::filesDropped(const juce::StringArray& files, int x, int y) {
    isExternalFileDragging = false;
    PlaylistDropHandler::processExternalFiles(files, x, y, *this);
}

bool PlaylistComponent::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) {
    juce::String desc = dragSourceDetails.description.toString();
    return desc.startsWith("PickerDrag|") || desc == "FileBrowserDrag";
}

void PlaylistComponent::itemDragEnter(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) {
    isInternalDragging = true;
    repaint();
}

void PlaylistComponent::itemDragExit(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) {
    isInternalDragging = false;
    repaint();
}

void PlaylistComponent::itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) {
    isInternalDragging = false;
    PlaylistDropHandler::processInternalItem(dragSourceDetails, *this);
}