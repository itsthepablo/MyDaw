#include "PlaylistComponent.h"
#include "../UI/WaveformRenderer.h" 
#include "../UI/MidiClipRenderer.h"
#include "Tools/PointerTool.h" 
#include "Tools/ScissorTool.h" 
#include "Tools/EraserTool.h" 
#include "Tools/MuteTool.h"
#include "PlaylistDropHandler.h"
#include "PlaylistActionHandler.h"
#include <cmath>
#include <algorithm>

PlaylistComponent::PlaylistComponent() {
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    activeTool = std::make_unique<PointerTool>();

    addAndMakeVisible(menuBar);
    addAndMakeVisible(hNavigator);
    addAndMakeVisible(vBar);

    menuBar.onToolChanged = [this](int toolId) { setTool(toolId); };
    menuBar.onSnapChanged = [this](double snap) { snapPixels = snap; repaint(); };
    menuBar.onUndo = [this] {
        // TODO: Llamar al motor de Undo local 
    };
    menuBar.onRedo = [this] {
        // TODO: Llamar al motor de Redo local
    };

    hNavigator.onScrollMoved = [this](double) { repaint(); };
    hNavigator.onZoomChanged = [this](double newZoom, double newStart) {
        hZoom = newZoom;
        hNavigator.setRangeLimits(0.0, getTimelineLength() * hZoom);
        hNavigator.setCurrentRange(newStart, (double)(getWidth() - vBarWidth));
        updateScrollBars();
        repaint();
        };

    hNavigator.onDrawMinimap = [this](juce::Graphics& g, juce::Rectangle<int> bounds) {
        this->drawMinimap(g, bounds);
        };

    vBar.onScrollMoved = [this](double start) {
        if (onVerticalScroll) onVerticalScroll((int)start);
        repaint();
        };

    updateScrollBars();
    startTimerHz(60);
}

PlaylistComponent::~PlaylistComponent() {
    removeKeyListener(this);
    stopTimer();
}

void PlaylistComponent::timerCallback() {
    // CORRECCIÓN DE RACE CONDITION: Solo lee el motor si está en Play
    if (isPlaying && getPlaybackPosition) {
        float newPos = getPlaybackPosition();
        if (playheadAbsPos != newPos) {
            playheadAbsPos = newPos;
            repaint();
        }
    }
}

void PlaylistComponent::updateScrollBars() {
    hNavigator.setZoomContext(hZoom, getTimelineLength());
    hNavigator.setRangeLimits(0.0, getTimelineLength() * hZoom);
    hNavigator.setCurrentRange(hNavigator.getCurrentRangeStart(), (double)(getWidth() - vBarWidth));

    int totalH = 0;
    if (tracksRef) {
        for (auto* t : *tracksRef) if (t->isShowingInChildren) totalH += (int)trackHeight;
    }

    int topOffset = menuBarH + navigatorH + timelineH;
    double visibleH = (double)getHeight() - topOffset;

    vBar.setRangeLimits(0.0, (double)totalH);
    vBar.setCurrentRange(vBar.getCurrentRangeStart(), visibleH);
}

void PlaylistComponent::addMidiClipToView(Track* targetTrack, MidiClipData* newClip) {
    clips.push_back({ targetTrack, newClip->startX, newClip->width, newClip->name, nullptr, newClip });
    updateScrollBars();
    repaint();
    hNavigator.repaint();
}

void PlaylistComponent::drawMinimap(juce::Graphics& g, juce::Rectangle<int> bounds) {
    if (!tracksRef || tracksRef->isEmpty()) return;

    double maxTime = getTimelineLength();
    double scaleX = bounds.getWidth() / maxTime;

    int visibleTracks = 0;
    for (auto* t : *tracksRef) {
        if (t->isShowingInChildren) visibleTracks++;
    }
    if (visibleTracks == 0) visibleTracks = 1;

    float trackMinimapH = std::min(6.0f, (float)bounds.getHeight() / visibleTracks);
    float totalOccupiedH = trackMinimapH * visibleTracks;
    float startY = bounds.getY() + (bounds.getHeight() - totalOccupiedH) / 2.0f;

    for (const auto& clip : clips) {
        int tIdx = -1;
        int currentVisIdx = 0;
        for (int i = 0; i < tracksRef->size(); ++i) {
            if ((*tracksRef)[i] == clip.trackPtr) {
                tIdx = currentVisIdx;
                break;
            }
            if ((*tracksRef)[i]->isShowingInChildren) currentVisIdx++;
        }
        if (tIdx == -1) continue;

        float x = bounds.getX() + (clip.startX * scaleX);
        float w = clip.width * scaleX;
        float y = startY + (tIdx * trackMinimapH);

        g.setColour(clip.trackPtr->getColor().withAlpha(0.8f));
        float padY = trackMinimapH > 3.0f ? 1.0f : 0.0f;
        g.fillRoundedRectangle(x, y + padY, juce::jmax(1.0f, w), juce::jmax(1.0f, trackMinimapH - (padY * 2.0f)), 1.5f);
    }
}

int PlaylistComponent::getTrackY(Track* targetTrack) const {
    if (!tracksRef) return -1;
    int topOffset = menuBarH + navigatorH + timelineH;
    int currentY = topOffset - (int)vBar.getCurrentRangeStart();
    for (auto* t : *tracksRef) {
        if (t == targetTrack) return currentY;
        if (t->isShowingInChildren) currentY += (int)trackHeight;
    }
    return -1;
}

int PlaylistComponent::getTrackAtY(int y) const {
    int topOffset = menuBarH + navigatorH + timelineH;
    if (y < topOffset) return -1;

    int vS = (int)vBar.getCurrentRangeStart();
    int currentY = topOffset - vS;

    if (!tracksRef) return -1;
    for (int i = 0; i < (int)tracksRef->size(); ++i) {
        auto* t = (*tracksRef)[i];
        if (!t->isShowingInChildren) continue;
        if (y >= currentY && y < currentY + trackHeight) return i;
        currentY += (int)trackHeight;
    }
    return -1;
}

void PlaylistComponent::deleteClip(int index) { PlaylistActionHandler::deleteClip(*this, index); }
void PlaylistComponent::deleteClipsByName(const juce::String& name, bool isMidi) { PlaylistActionHandler::deleteClipsByName(*this, name, isMidi); }
void PlaylistComponent::purgeClipsOfTrack(Track* track) { PlaylistActionHandler::purgeClipsOfTrack(*this, track); }
void PlaylistComponent::mouseDoubleClick(const juce::MouseEvent& e) { PlaylistActionHandler::handleDoubleClick(*this, e); }

void PlaylistComponent::setTool(int toolId) {
    if (toolId == 1) activeTool = std::make_unique<PointerTool>();
    else if (toolId == 3) activeTool = std::make_unique<ScissorTool>();
    else if (toolId == 4) activeTool = std::make_unique<EraserTool>();
    else if (toolId == 5) activeTool = std::make_unique<MuteTool>();
}

bool PlaylistComponent::keyPressed(const juce::KeyPress& key, juce::Component*) {
    if (key.getKeyCode() == juce::KeyPress::deleteKey || key.getKeyCode() == juce::KeyPress::backspaceKey) {
        if (selectedClipIndex != -1) { deleteClip(selectedClipIndex); return true; }
    }
    return false;
}

void PlaylistComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(25, 27, 30));
    float hS = (float)hNavigator.getCurrentRangeStart();
    float vS = (float)vBar.getCurrentRangeStart();

    int topOffset = menuBarH + navigatorH + timelineH;
    int viewAreaH = getHeight() - topOffset;

    g.saveState();
    g.reduceClipRegion(0, topOffset, getWidth() - (vBar.isVisible() ? vBarWidth : 0), viewAreaH);

    double visualSnap = (snapPixels < 10.0) ? 80.0 : snapPixels;

    for (double i = 0; i <= getTimelineLength(); i += visualSnap) {
        int dx = (int)(i * hZoom) - (int)hS;
        if (dx < 0) continue;

        if (std::fmod(i, 80.0) == 0.0) g.setColour(juce::Colours::white.withAlpha(0.1f));
        else g.setColour(juce::Colours::white.withAlpha(0.04f));

        g.drawVerticalLine(dx, (float)topOffset, (float)getHeight());
    }

    int currentY = topOffset - (int)vS;
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
        if (yPos < topOffset - 100 || yPos > getHeight()) continue;

        int xPos = (int)(clip.startX * hZoom) - (int)hS;
        int wPos = (int)(clip.width * hZoom);
        juce::Rectangle<int> clipRect(xPos, yPos + 2, wPos - 1, (int)trackHeight - 4);

        g.setColour(clip.trackPtr->getColor().withAlpha(0.2f));
        g.fillRoundedRectangle(clipRect.toFloat(), 4.0f);

        bool isMutedLocally = (clip.linkedAudio && clip.linkedAudio->isMuted) || (clip.linkedMidi && clip.linkedMidi->isMuted);
        float alphaMult = isMutedLocally ? 0.3f : 1.0f;

        g.setOpacity(alphaMult);

        if (clip.linkedAudio != nullptr) {
            WaveformRenderer::drawWaveform(g, *clip.linkedAudio, clipRect, clip.trackPtr->getColor(), clip.trackPtr->getWaveformViewMode());
        }

        if (clip.linkedMidi != nullptr) {
            MidiClipRenderer::drawMidiClip(g, *clip.linkedMidi, clipRect, clip.trackPtr->getColor(), clip.trackPtr->isInlineEditingActive, hZoom, hS);
        }

        g.setColour(juce::Colours::white.withAlpha(0.9f * alphaMult));
        g.drawText(clip.name, clipRect.reduced(5, 2), juce::Justification::topLeft, true);
        
        g.setOpacity(1.0f);

        if (i == selectedClipIndex) {
            g.setColour(juce::Colours::white);
            g.drawRoundedRectangle(clipRect.toFloat(), 4.0f, 1.5f);
        }
    }
    g.restoreState();

    g.setColour(juce::Colour(20, 22, 25));
    g.fillRect(0, menuBarH + navigatorH, getWidth(), timelineH);

    int phX = (int)(playheadAbsPos * hZoom) - (int)hS;
    g.setColour(juce::Colours::red);
    g.drawVerticalLine(phX, (float)(menuBarH + navigatorH), (float)getHeight());

    if (isExternalFileDragging || isInternalDragging) {
        g.fillAll(juce::Colours::dodgerblue.withAlpha(0.2f));
        g.setColour(juce::Colours::white);
        juce::String overlayText = isInternalDragging ? "SUELTA EL CLIP AQUI" : "SUELTA TU AUDIO AQUI";
        g.drawText(overlayText, getLocalBounds(), juce::Justification::centred);
    }
}

void PlaylistComponent::resized() {
    menuBar.setBounds(0, 0, getWidth(), menuBarH);
    hNavigator.setBounds(0, menuBarH, getWidth() - vBarWidth, navigatorH);
    vBar.setBounds(getWidth() - vBarWidth, menuBarH + navigatorH, vBarWidth, getHeight() - (menuBarH + navigatorH));
    updateScrollBars();
}

void PlaylistComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) {
    if (e.mods.isCtrlDown()) {
        double mouseAbsX = getAbsoluteXFromMouse(e.x);

        hZoom = juce::jlimit(0.01, 5.0, hZoom + (double)w.deltaY * 0.1);
        double newStart = (mouseAbsX * hZoom) - e.x;

        hNavigator.setRangeLimits(0.0, getTimelineLength() * hZoom);
        hNavigator.setCurrentRange(newStart, (double)(getWidth() - vBarWidth));
        updateScrollBars();
    }
    repaint();
}

int PlaylistComponent::getClipAt(int x, int y) const {
    int tIdx = getTrackAtY(y); if (tIdx == -1) return -1;
    float absX = getAbsoluteXFromMouse(x);
    for (int i = (int)clips.size() - 1; i >= 0; --i) {
        if (clips[i].trackPtr == (*tracksRef)[tIdx] && absX >= clips[i].startX && absX <= clips[i].startX + clips[i].width) return i;
    }
    return -1;
}

void PlaylistComponent::mouseDown(const juce::MouseEvent& e) { if (activeTool) activeTool->mouseDown(e, *this); }
void PlaylistComponent::mouseDrag(const juce::MouseEvent& e) { if (activeTool) activeTool->mouseDrag(e, *this); }
void PlaylistComponent::mouseUp(const juce::MouseEvent& e) { if (activeTool) activeTool->mouseUp(e, *this); }
void PlaylistComponent::mouseMove(const juce::MouseEvent& e) { if (activeTool) activeTool->mouseMove(e, *this); }

bool PlaylistComponent::isInterestedInFileDrag(const juce::StringArray&) { return true; }
void PlaylistComponent::fileDragEnter(const juce::StringArray&, int, int) { isExternalFileDragging = true; repaint(); }
void PlaylistComponent::fileDragExit(const juce::StringArray&) { isExternalFileDragging = false; repaint(); }
void PlaylistComponent::filesDropped(const juce::StringArray& files, int x, int y) {
    isExternalFileDragging = false;
    PlaylistDropHandler::processExternalFiles(files, x, y, *this);
}

bool PlaylistComponent::isInterestedInDragSource(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) {
    juce::String desc = dragSourceDetails.description.toString();
    return desc.startsWith("PickerDrag|") || desc == "FileBrowserDrag";
}
void PlaylistComponent::itemDragEnter(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) { isInternalDragging = true; repaint(); }
void PlaylistComponent::itemDragExit(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) { isInternalDragging = false; repaint(); }
void PlaylistComponent::itemDropped(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails) {
    isInternalDragging = false;
    PlaylistDropHandler::processInternalItem(dragSourceDetails, *this);
}