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
    // CORRECCIÓN DE RACE CONDITION: Solo lee el motor si está en Play y NO se está arrastrando el timeline
    if (isPlaying && getPlaybackPosition && !isDraggingTimeline) {
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

    // RESTAMOS 60px DE ALTURA PARA QUE LA ÚLTIMA PISTA NO QUEDE DETRÁS DEL MASTER STRIP
    double visibleH = (double)getHeight() - topOffset - 60.0;

    vBar.setRangeLimits(0.0, (double)totalH);
    vBar.setCurrentRange(vBar.getCurrentRangeStart(), visibleH);

    if (onVerticalScroll) onVerticalScroll((int)vBar.getCurrentRangeStart());
}

void PlaylistComponent::addMidiClipToView(Track* targetTrack, MidiClipData* newClip) {
    clips.push_back({ targetTrack, newClip->startX, newClip->width, newClip->name, nullptr, newClip });
    if (targetTrack != nullptr) targetTrack->commitSnapshot(); // DOUBLE BUFFER: clip MIDI añadido desde Piano Roll
    updateScrollBars();
    repaint();
    hNavigator.repaint();
}

void PlaylistComponent::addAudioClipToView(Track* targetTrack, AudioClipData* newClip) {
    clips.push_back({ targetTrack, newClip->startX, newClip->width, newClip->name, newClip, nullptr });
    if (targetTrack != nullptr) targetTrack->commitSnapshot(); 
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
            auto* t = (*tracksRef)[i];
            if (t == clip.trackPtr) {
                if (!t->isShowingInChildren) { tIdx = -1; break; } // CLIP OCULTO
                tIdx = currentVisIdx;
                break;
            }
            if (t->isShowingInChildren) currentVisIdx++;
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
    if (!tracksRef || !targetTrack) return -1;
    
    // BUG FIX: Si el track está oculto por una carpeta, los clips NO se dibujan (coordenada Y = -1)
    if (!targetTrack->isShowingInChildren) return -1;

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

void PlaylistComponent::deleteSelectedClips() { PlaylistActionHandler::deleteSelectedClips(*this); }
void PlaylistComponent::deleteClipsByName(const juce::String& name, bool isMidi) { PlaylistActionHandler::deleteClipsByName(*this, name, isMidi); }
void PlaylistComponent::purgeClipsOfTrack(Track* track) { PlaylistActionHandler::purgeClipsOfTrack(*this, track); }
void PlaylistComponent::mouseDoubleClick(const juce::MouseEvent& e) { PlaylistActionHandler::handleDoubleClick(*this, e); }

void PlaylistComponent::setTool(int toolId) {
    if (toolId == 1) activeTool = std::make_unique<PointerTool>();
    else if (toolId == 3) activeTool = std::make_unique<ScissorTool>();
    else if (toolId == 4) activeTool = std::make_unique<EraserTool>();
    else if (toolId == 5) activeTool = std::make_unique<MuteTool>();
}

bool PlaylistComponent::keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) {
    if (key.getKeyCode() == juce::KeyPress::deleteKey || key.getKeyCode() == juce::KeyPress::backspaceKey) {
        if (!selectedClipIndices.empty()) { deleteSelectedClips(); return true; }
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

    // 1. Dibujar el fondo alternado cada 4 compases (FL Studio Style)
    double blockLengthPx = 1280.0; // 4 compases * 320px
    double startTime = (double)hS / hZoom;
    double endTime = (double)(hS + getWidth()) / hZoom;

    int startBlock = (int)(startTime / blockLengthPx);
    int endBlock = (int)(endTime / blockLengthPx);

    for (int b = startBlock; b <= endBlock; ++b) {
        if (b % 2 != 0) { // Solo pintar el alternado (el par ya es color base)
            double blockAbsX = b * blockLengthPx;
            int xPos = (int)(blockAbsX * hZoom) - (int)hS;
            int wPos = (int)(std::ceil(blockLengthPx * hZoom));

            g.setColour(juce::Colour(32, 34, 38)); // Gris ligeramente más iluminado
            g.fillRect(xPos, topOffset, wPos, viewAreaH);
        }
    }

    // 2. Líneas verticales dinámicas y Culling
    double visualSnap = (snapPixels < 10.0) ? 80.0 : snapPixels;
    double currentDrawSnap = visualSnap;

    // Limitador de congestión
    while ((currentDrawSnap * hZoom) < 8.0 && currentDrawSnap < 320.0) {
        currentDrawSnap *= 2.0;
    }
    if (currentDrawSnap >= 320.0) {
        double pxPerMeasure = 320.0 * hZoom;
        if (pxPerMeasure < 5.0) currentDrawSnap = 320.0 * 16.0;
        else if (pxPerMeasure < 10.0) currentDrawSnap = 320.0 * 8.0;
        else if (pxPerMeasure < 20.0) currentDrawSnap = 320.0 * 4.0;
        else if (pxPerMeasure < 40.0) currentDrawSnap = 320.0 * 2.0;
    }

    double startLineSearch = std::floor(startTime / currentDrawSnap) * currentDrawSnap;
    double endLineSearch = std::min(getTimelineLength(), endTime);

    for (double i = startLineSearch; i <= endLineSearch; i += currentDrawSnap) {
        int dx = (int)(i * hZoom) - (int)hS;

        if (std::fmod(i, 320.0) == 0.0) g.setColour(juce::Colours::black);
        else if (std::fmod(i, 80.0) == 0.0) g.setColour(juce::Colours::black);
        else g.setColour(juce::Colours::black);

        g.drawVerticalLine(dx, (float)topOffset, (float)getHeight());
    }

    int currentY = topOffset - (int)vS;
    if (tracksRef) {
        for (auto* t : *tracksRef) {
            if (!t->isShowingInChildren) continue;
            g.setColour(juce::Colours::black.withAlpha(0.8f)); // Separador mucho más oscuro
            g.fillRect(0.0f, (float)(currentY + (int)trackHeight - 2), (float)getWidth(), 2.0f); // 2 píxeles de grosor puro
            currentY += (int)trackHeight;
        }
    }

    for (int i = 0; i < (int)clips.size(); ++i) {
        const auto& clip = clips[i];
        int yPos = getTrackY(clip.trackPtr);
        
        // BUG FIX: Si el track está oculto por una carpeta, yPos será -1. Ignoramos este clip.
        if (yPos == -1) continue;

        int clipTop = yPos;
        int clipBottom = yPos + (int)trackHeight;

        // Culling vertical correcto: omitimos si el clip completo está por encima o por debajo de la ventana
        if (clipBottom < topOffset || clipTop > getHeight()) continue;

        int xPos = (int)(clip.startX * hZoom) - (int)hS;
        int wPos = (int)(clip.width * hZoom);
        juce::Rectangle<int> clipRect(xPos, yPos + 2, wPos - 1, (int)trackHeight - 4);

        juce::Colour trackColor = clip.trackPtr->getColor();

        bool isMutedLocally = (clip.linkedAudio && clip.linkedAudio->isMuted) || (clip.linkedMidi && clip.linkedMidi->isMuted);
        float alphaMult = isMutedLocally ? 0.3f : 1.0f;
        g.setOpacity(alphaMult);

        // 1. Fondo completo del clip (muy oscuro)
        g.setColour(trackColor.darker(0.8f).withAlpha(1.0f));
        g.fillRoundedRectangle(clipRect.toFloat(), 5.0f);

        // 2. Cabecera superior (Header) solo para el texto sin oscurecer base
        juce::Rectangle<int> headerRect = clipRect.withHeight(18); // 18px de altura fija
        g.setColour(trackColor.darker(0.8f).withAlpha(1.0f));
        g.fillRoundedRectangle(headerRect.toFloat(), 5.0f);
        if (headerRect.getHeight() > 5) g.fillRect(headerRect.withTop(headerRect.getBottom() - 5).toFloat()); // Quitar curvas inferiores

        // 3. Renderizar Onda o MIDI debajo del header
        juce::Rectangle<int> innerArea = clipRect;
        innerArea.removeFromTop(18); // Se corre la zona de renderizado

        if (clip.linkedAudio != nullptr) {
            WaveformRenderer::drawWaveform(g, *clip.linkedAudio, innerArea, trackColor, clip.trackPtr->getWaveformViewMode(), hZoom);
        }
        if (clip.linkedMidi != nullptr) {
            MidiClipRenderer::drawMidiClip(g, *clip.linkedMidi, innerArea, trackColor, clip.trackPtr->isInlineEditingActive, hZoom, hS);
        }

        // 4. Escribir texto en su cabecera exclusiva
        g.setColour(juce::Colours::white);
        g.drawText(" " + clip.name, headerRect.reduced(3, 0), juce::Justification::centredLeft, true);

        g.setOpacity(1.0f);

        // Contorno de selección multi
        if (std::find(selectedClipIndices.begin(), selectedClipIndices.end(), i) != selectedClipIndices.end()) {
            g.setColour(juce::Colours::yellow);
            g.drawRoundedRectangle(clipRect.toFloat(), 5.0f, 1.5f);
        }
    }
    g.restoreState();

    g.setColour(juce::Colour(20, 22, 25));
    g.fillRect(0, menuBarH + navigatorH, getWidth(), timelineH);

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(12.0f);

    double pixelsPerMeasure = 320.0 * hZoom;
    int measureMod = 1;
    if (pixelsPerMeasure < 15.0) measureMod = 16;
    else if (pixelsPerMeasure < 30.0) measureMod = 8;
    else if (pixelsPerMeasure < 60.0) measureMod = 4;
    else if (pixelsPerMeasure < 120.0) measureMod = 2;

    for (double i = 0; i <= getTimelineLength(); i += 80.0) {
        int dx = (int)(i * hZoom) - (int)hS;
        if (dx < 0 || dx > getWidth()) continue;

        if (std::fmod(i, 320.0) == 0.0) {
            int measureNumber = (int)(i / 320.0) + 1;
            g.drawVerticalLine(dx, (float)(menuBarH + navigatorH + timelineH - 12), (float)(menuBarH + navigatorH + timelineH));

            if (measureMod == 1 || (measureNumber - 1) % measureMod == 0) {
                g.drawText(juce::String(measureNumber), dx + 4, menuBarH + navigatorH, 40, timelineH, juce::Justification::centredLeft, false);
            }
        }
        else {
            if (pixelsPerMeasure >= 30.0) {
                g.drawVerticalLine(dx, (float)(menuBarH + navigatorH + timelineH - 6), (float)(menuBarH + navigatorH + timelineH));
            }
        }
    }

    int phX = (int)(playheadAbsPos * hZoom) - (int)hS;
    if (phX >= 0 && phX <= getWidth()) {
        float phTop = (float)(menuBarH + navigatorH);
        float phBottom = (float)getHeight();

        // 1. Cola difuminada (Fade estela)
        int tailWidth = 40; // Pixels
        juce::Rectangle<float> tailRect(phX - tailWidth, phTop, tailWidth, phBottom - phTop);
        juce::ColourGradient tailGrad(
            juce::Colours::transparentWhite, phX - tailWidth, 0.0f,
            juce::Colours::white.withAlpha(0.15f), phX, 0.0f,
            false // horizontal
        );
        g.setGradientFill(tailGrad);
        g.fillRect(tailRect);

        // 2. Línea principal (Blanca)
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.drawVerticalLine(phX, phTop, phBottom);

        // 3. Triángulo invertido en el timeline (simétrico)
        juce::Path triangle;
        float triTopY = phTop;
        float triHeight = 8.0f;
        float triHalfWidth = 6.0f;

        triangle.addTriangle(
            phX - triHalfWidth, triTopY,
            phX + triHalfWidth, triTopY,
            phX, triTopY + triHeight
        );

        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.fillPath(triangle);
    }

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
        // Zoom Horizontal Detallado - Centrado en el mouse
        double mouseAbsX = getAbsoluteXFromMouse(e.x);
        double zoomFactor = (w.deltaY > 0) ? 1.15 : (1.0 / 1.15);
        if (w.isReversed) zoomFactor = 1.0 / zoomFactor;
        hZoom = juce::jlimit(0.05, 10.0, hZoom * zoomFactor);
        double newStart = (mouseAbsX * hZoom) - e.x;
        hNavigator.setRangeLimits(0.0, getTimelineLength() * hZoom);
        hNavigator.setCurrentRange(newStart, (double)(getWidth() - vBarWidth));
        updateScrollBars();
    }
    else if (e.mods.isShiftDown()) {
        // Scroll Horizontal Estándar
        double newStart = hNavigator.getCurrentRangeStart() - (w.deltaY * 100.0);
        hNavigator.setCurrentRange(newStart, (double)(getWidth() - vBarWidth));
        updateScrollBars();
    }
    else if (e.mods.isAltDown()) {
        // Zoom Vertical Detallado - Centrado en el mouse (Reaper style)
        double currentMouseY = e.y;
        int topOffset = menuBarH + navigatorH + timelineH;
        if (currentMouseY > topOffset) {
            double vS = vBar.getCurrentRangeStart();
            double mouseAbsY = vS + (currentMouseY - topOffset);
            double zoomFactor = (w.deltaY > 0) ? 1.15 : (1.0 / 1.15);
            if (w.isReversed) zoomFactor = 1.0 / zoomFactor;
            double oldTrackHeight = trackHeight;
            trackHeight = (float)juce::jlimit(30.0, 400.0, trackHeight * zoomFactor);
            if (trackContainer) trackContainer->setTrackHeight(trackHeight);
            double heightRatio = trackHeight / oldTrackHeight;
            double newVS = mouseAbsY * heightRatio - (currentMouseY - topOffset);
            vBar.setCurrentRange(newVS, (double)(getHeight() - topOffset));
            updateScrollBars();
        }
    }
    else {
        // Scroll Vertical Estándar
        int totalH = 0;
        if (tracksRef) {
            for (auto* t : *tracksRef) if (t->isShowingInChildren) totalH += (int)trackHeight;
        }
        int topOffset = menuBarH + navigatorH + timelineH;
        double newStart = juce::jlimit(0.0, juce::jmax(0.0, (double)totalH - ((double)getHeight() - topOffset - 60.0)), vBar.getCurrentRangeStart() - (w.deltaY * 100.0));
        vBar.setCurrentRange(newStart, (double)(getHeight() - topOffset - 60.0));
        updateScrollBars();
        repaint();
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

void PlaylistComponent::mouseDown(const juce::MouseEvent& e) {
    if (e.y >= menuBarH + navigatorH && e.y < menuBarH + navigatorH + timelineH) {
        isDraggingTimeline = true;
        float newPos = juce::jmax(0.0f, getAbsoluteXFromMouse(e.x));
        setPlayheadPos(newPos);
        if (onPlayheadSeekRequested) onPlayheadSeekRequested(newPos);
        return;
    }
    if (activeTool) activeTool->mouseDown(e, *this);
}

void PlaylistComponent::mouseDrag(const juce::MouseEvent& e) {
    if (isDraggingTimeline) {
        float newPos = juce::jmax(0.0f, getAbsoluteXFromMouse(e.x));
        setPlayheadPos(newPos);
        if (onPlayheadSeekRequested) onPlayheadSeekRequested(newPos);
        return;
    }
    if (activeTool) activeTool->mouseDrag(e, *this);
}

void PlaylistComponent::mouseUp(const juce::MouseEvent& e) {
    if (isDraggingTimeline) { isDraggingTimeline = false; return; }
    if (activeTool) activeTool->mouseUp(e, *this);
}

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