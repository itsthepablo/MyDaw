#include "PlaylistComponent.h"
#include "../Theme/CustomTheme.h"
#include "../UI/AutomationRenderer.h"
#include "../UI/MidiClipRenderer.h"
#include "../UI/MidiPatternStyles.h"
#include "../UI/WaveformRenderer.h"
#include "PlaylistActionHandler.h"
#include "PlaylistDropHandler.h"
#include "../UI/Playlist/LoudnessRenderer.h"
#include "../UI/Playlist/BalanceRenderer.h"
#include "../UI/Playlist/MidSideRenderer.h"
#include "Tools/EraserTool.h"
#include "Tools/MuteTool.h"
#include "Tools/PointerTool.h"
#include "Tools/ScissorTool.h"
#include "PlaylistCoordinateUtils.h"
#include "PlaylistAnalyzer.h"
#include "View/PlaylistGridRenderer.h"
#include "View/PlaylistRenderer.h"
#include <algorithm>
#include <cmath>
#include <unordered_map>



PlaylistComponent::PlaylistComponent() {
  setWantsKeyboardFocus(true);
  addKeyListener(this);

  activeTool = std::make_unique<PointerTool>();

  addAndMakeVisible(menuBar);
  addAndMakeVisible(hNavigator);
  addAndMakeVisible(vBar);

  menuBar.onToolChanged = [this](int toolId) { setTool(toolId); };
  menuBar.onSnapChanged = [this](double snap) {
    snapPixels = snap;
    repaint();
  };
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

  hNavigator.onDrawMinimap = [this](juce::Graphics &g,
                                    juce::Rectangle<int> bounds) {
    this->drawMinimap(g, bounds);
  };

  vBar.onScrollMoved = [this](double start) {
    if (onVerticalScroll)
      onVerticalScroll((int)start);
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
  // CORRECCIÓN DE RACE CONDITION: Solo lee el motor si está en Play y NO se
  // está arrastrando el timeline
  if (isPlaying && getPlaybackPosition && !isDraggingTimeline) {
    float newPos = getPlaybackPosition();
    if (playheadAbsPos != newPos) {
      playheadAbsPos = newPos;

      // Delegación al Analyzer para grabación de historial (Loudness, Balance, MS)
      PlaylistAnalyzer::recordAnalysisData(tracksRef, masterTrackPtr, playheadAbsPos, isDraggingTimeline);
      
      repaint();
    }
  }
}


void PlaylistComponent::updateScrollBars() {
  hNavigator.setZoomContext(hZoom, getTimelineLength());
  hNavigator.setRangeLimits(0.0, getTimelineLength() * hZoom);
  hNavigator.setCurrentRange(hNavigator.getCurrentRangeStart(),
                             (double)(getWidth() - vBarWidth));

  int totalH = 0;
  if (tracksRef) {
    for (auto *t : *tracksRef)
      if (t->isShowingInChildren)
        totalH += (int)trackHeight;
  }

  int topOffset = menuBarH + navigatorH + timelineH;

  // RESTAMOS LA ALTURA DEL MASTER TRACK PARA QUE LA ÚLTIMA PISTA NO QUEDE DETRÁS
  double visibleH = (double)getHeight() - topOffset - (double)masterTrackH;

  vBar.setRangeLimits(0.0, (double)totalH);
  vBar.setCurrentRange(vBar.getCurrentRangeStart(), visibleH);

  if (onVerticalScroll)
    onVerticalScroll((int)vBar.getCurrentRangeStart());
}

void PlaylistComponent::addMidiClipToView(Track *targetTrack,
                                          MidiClipData *newClip) {
  clips.push_back({targetTrack, newClip->startX, newClip->width, newClip->name,
                   nullptr, newClip});
  if (targetTrack != nullptr)
    targetTrack
        ->commitSnapshot(); // DOUBLE BUFFER: clip MIDI añadido desde Piano Roll
  updateScrollBars();
  repaint();
  hNavigator.repaint();
}

void PlaylistComponent::addAudioClipToView(Track *targetTrack,
                                           AudioClipData *newClip) {
  clips.push_back({targetTrack, newClip->startX, newClip->width, newClip->name,
                   newClip, nullptr});
  if (targetTrack != nullptr)
    targetTrack->commitSnapshot();
  updateScrollBars();
  repaint();
  hNavigator.repaint();
}

void PlaylistComponent::drawMinimap(juce::Graphics &g,
                                    juce::Rectangle<int> bounds) {
  PlaylistRenderer::drawMinimap(g, bounds, tracksRef, clips, getTimelineLength());
}


int PlaylistComponent::getTrackY(Track *targetTrack) const {
  return PlaylistCoordinateUtils::getTrackY(targetTrack, tracksRef, 
                                            menuBarH + navigatorH + timelineH, 
                                            vBar, trackHeight);
}

int PlaylistComponent::getTrackAtY(int y) const {
  return PlaylistCoordinateUtils::getTrackAtY(y, menuBarH + navigatorH + timelineH, 
                                              vBar, tracksRef, trackHeight);
}


void PlaylistComponent::deleteSelectedClips() {
  PlaylistActionHandler::deleteSelectedClips(*this);
}
void PlaylistComponent::deleteClipsByName(const juce::String &name,
                                          bool isMidi) {
  PlaylistActionHandler::deleteClipsByName(*this, name, isMidi);
}
void PlaylistComponent::purgeClipsOfTrack(Track *track) {
  PlaylistActionHandler::purgeClipsOfTrack(*this, track);
}
void PlaylistComponent::mouseDoubleClick(const juce::MouseEvent &e) {
  PlaylistActionHandler::handleDoubleClick(*this, e);
}

void PlaylistComponent::setTool(int toolId) {
  if (toolId == 1)
    activeTool = std::make_unique<PointerTool>();
  else if (toolId == 3)
    activeTool = std::make_unique<ScissorTool>();
  else if (toolId == 4)
    activeTool = std::make_unique<EraserTool>();
  else if (toolId == 5)
    activeTool = std::make_unique<MuteTool>();
}

bool PlaylistComponent::keyPressed(const juce::KeyPress &key,
                                   juce::Component *originatingComponent) {
  if (key.getKeyCode() == juce::KeyPress::deleteKey ||
      key.getKeyCode() == juce::KeyPress::backspaceKey) {
    if (!selectedClipIndices.empty()) {
      deleteSelectedClips();
      return true;
    }
  }
  return false;
}

void PlaylistComponent::paint(juce::Graphics &g) {
  if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
    g.fillAll(theme->getSkinColor("PLAYLIST_BG", juce::Colour(25, 27, 30)));
  } else {
    g.fillAll(juce::Colour(25, 27, 30));
  }

  float hS = (float)hNavigator.getCurrentRangeStart();
  float vS = (float)vBar.getCurrentRangeStart();
  int topOffset = menuBarH + navigatorH + timelineH;
  int viewAreaH = getHeight() - topOffset - masterTrackH; // Descontar el Master Lane fijo

  // 1. Dibujar el fondo alternado y la rejilla vertical (TODO EL PROYECTO)
  PlaylistGridRenderer::drawVerticalGrid(g, topOffset, getHeight(), getWidth(), 
                                         hS, hZoom, snapPixels, getTimelineLength(), getLookAndFeel());

  // --- RECORTE PARA CONTENIDO QUE HACE SCROLL (Pistas normales) ---
  g.saveState();
  juce::Rectangle<int> playlistBounds(0, topOffset, 
                                      getWidth() - (vBar.isVisible() ? vBarWidth : 0), 
                                      viewAreaH);
  g.reduceClipRegion(playlistBounds);

  // --- PRE-CÁLCULO DE CACHÉ DE COORDENADAS Y ---
  std::unordered_map<Track *, int> trackYCache;
  int ty = topOffset - (int)vS;
  if (tracksRef) {
    for (auto *t : *tracksRef) {
      if (t->isShowingInChildren) {
        trackYCache[t] = ty;
        ty += (int)trackHeight;
      }
    }
  }

  // 2. Dibujar separadores de pistas (SÓLO DENTRO DEL CLIP REGION)
  PlaylistGridRenderer::drawHorizontalSeparators(g, topOffset, viewAreaH, getWidth(), 
                                                tracksRef, vS, trackHeight);

  // Preparar el contexto para el renderizado de clips y otros elementos
  PlaylistViewContext ctx = {
    tracksRef, &clips, hZoom, hS, vS, trackHeight, topOffset, getWidth(), getHeight(),
    playheadAbsPos, &selectedClipIndices, hoveredAutoClip, 
    isExternalFileDragging, isInternalDragging
  };

  // 3. Dibujar resúmenes de carpeta (Reaper style)
  PlaylistRenderer::drawFolderSummaries(g, ctx, trackYCache);

  // 4. Dibujar clips de audio y MIDI
  PlaylistRenderer::drawTracksAndClips(g, ctx, trackYCache);

  // 5. Dibujar pistas de análisis (Loudness, Balance, MidSide)
  PlaylistRenderer::drawSpecialAnalysisTracks(g, ctx, trackYCache);

  // 6. Dibujar capas de automatización (Ableton Style)
  PlaylistRenderer::drawAutomationOverlays(g, ctx, trackYCache);

  g.restoreState();

  // 7. Dibujar la regla de tiempos (Ruler)
  PlaylistGridRenderer::drawTimelineRuler(g, menuBarH, navigatorH, timelineH, getWidth(), 
                                          hS, hZoom, getTimelineLength());

  // 8. Dibujar el cabezal de reproducción (Playhead)
  PlaylistGridRenderer::drawPlayhead(g, playheadAbsPos, hZoom, hS, menuBarH, navigatorH, 
                                     getWidth(), getHeight());

  // 9. Dibujar overlays de arrastre (Drag & Drop)
  PlaylistRenderer::drawDragOverlays(g, ctx);

  // 10. Dibujar Línea Divisoria del Master (Diseño Integrado)
  g.setColour(juce::Colours::white.withAlpha(0.15f));
  float masterY = (float)getHeight() - (float)masterTrackH;
  g.drawHorizontalLine((int)masterY, 0.0f, (float)getWidth());
}


void PlaylistComponent::resized() {
  menuBar.setBounds(0, 0, getWidth(), menuBarH);
  hNavigator.setBounds(0, menuBarH, getWidth() - vBarWidth, navigatorH);
  vBar.setBounds(getWidth() - vBarWidth, menuBarH + navigatorH, vBarWidth,
                 getHeight() - (menuBarH + navigatorH));
  updateScrollBars();
}

void PlaylistComponent::mouseWheelMove(const juce::MouseEvent &e,
                                       const juce::MouseWheelDetails &w) {
  if (e.mods.isCtrlDown()) {
    // Zoom Horizontal Detallado - Centrado en el mouse
    double mouseAbsX = getAbsoluteXFromMouse(e.x);
    double zoomFactor = (w.deltaY > 0) ? 1.15 : (1.0 / 1.15);
    if (w.isReversed)
      zoomFactor = 1.0 / zoomFactor;
    hZoom = juce::jlimit(0.01, 100000.0, hZoom * zoomFactor);
    double newStart = (mouseAbsX * hZoom) - e.x;
    hNavigator.setRangeLimits(0.0, getTimelineLength() * hZoom);
    hNavigator.setCurrentRange(newStart, (double)(getWidth() - vBarWidth));
    updateScrollBars();
  } else if (e.mods.isShiftDown()) {
    // Scroll Horizontal Estándar
    double newStart = hNavigator.getCurrentRangeStart() - (w.deltaY * 100.0);
    hNavigator.setCurrentRange(newStart, (double)(getWidth() - vBarWidth));
    updateScrollBars();
  } else if (e.mods.isAltDown()) {
    // Zoom Vertical Detallado - Centrado en el mouse (Reaper style)
    double currentMouseY = e.y;
    int topOffset = menuBarH + navigatorH + timelineH;
    if (currentMouseY > topOffset) {
      double vS = vBar.getCurrentRangeStart();
      double mouseAbsY = vS + (currentMouseY - topOffset);
      double zoomFactor = (w.deltaY > 0) ? 1.15 : (1.0 / 1.15);
      if (w.isReversed)
        zoomFactor = 1.0 / zoomFactor;
      double oldTrackHeight = trackHeight;
      trackHeight = (float)juce::jlimit(30.0, 400.0, trackHeight * zoomFactor);
      if (trackContainer)
        trackContainer->setTrackHeight(trackHeight);
      double heightRatio = trackHeight / oldTrackHeight;
      double newVS = mouseAbsY * heightRatio - (currentMouseY - topOffset);
      vBar.setCurrentRange(newVS, (double)(getHeight() - topOffset));
      updateScrollBars();
    }
  } else {
    // Scroll Vertical Estándar
    int totalH = 0;
    if (tracksRef) {
      for (auto *t : *tracksRef)
        if (t->isShowingInChildren)
          totalH += (int)trackHeight;
    }
    int topOffset = menuBarH + navigatorH + timelineH;
    double newStart =
        juce::jlimit<double>(0.0,
                             juce::jmax<double>(0.0, (double)totalH - ((double)getHeight() -
                                                               topOffset - (double)masterTrackH)),
                             vBar.getCurrentRangeStart() - (w.deltaY * 100.0));
    vBar.setCurrentRange(newStart, (double)getHeight() - topOffset - (double)masterTrackH);
    updateScrollBars();
    repaint();
  }
  repaint();
}

int PlaylistComponent::getClipAt(int x, int y) const {
  return PlaylistCoordinateUtils::getClipAt(x, y, hNavigator, hZoom, 
                                            menuBarH + navigatorH + timelineH, 
                                            vBar, tracksRef, trackHeight, clips);
}


void PlaylistComponent::mouseDown(const juce::MouseEvent &e) {
  if (e.y >= menuBarH + navigatorH && e.y < menuBarH + navigatorH + timelineH) {
    isDraggingTimeline = true;
    float newPos = juce::jmax(0.0f, getAbsoluteXFromMouse(e.x));
    setPlayheadPos(newPos);
    if (onPlayheadSeekRequested)
      onPlayheadSeekRequested(newPos);
    return;
  }
  if (activeTool)
    activeTool->mouseDown(e, *this);
}

void PlaylistComponent::mouseDrag(const juce::MouseEvent &e) {
  if (isDraggingTimeline) {
    float newPos = juce::jmax(0.0f, getAbsoluteXFromMouse(e.x));
    setPlayheadPos(newPos);
    if (onPlayheadSeekRequested)
      onPlayheadSeekRequested(newPos);
    return;
  }
  if (activeTool)
    activeTool->mouseDrag(e, *this);
}

void PlaylistComponent::mouseUp(const juce::MouseEvent &e) {
  if (isDraggingTimeline) {
    isDraggingTimeline = false;
    return;
  }
  if (activeTool)
    activeTool->mouseUp(e, *this);
}

void PlaylistComponent::mouseMove(const juce::MouseEvent &e) {
  AutomationClipData *newHover = nullptr;
  if (tracksRef) {
    for (auto *t : *tracksRef) {
      if (!t->isShowingInChildren)
        continue;
      for (auto *aut : t->automationClips) {
        if (aut && aut->isShowing) {
          int yPos = getTrackY(t);
          if (yPos != -1 && e.y >= yPos && e.y <= yPos + trackHeight) {
            newHover = aut;
            break;
          }
        }
      }
      if (newHover)
        break;
    }
  }
  if (hoveredAutoClip != newHover) {
    hoveredAutoClip = newHover;
    repaint();
  }
  if (activeTool)
    activeTool->mouseMove(e, *this);
}

void PlaylistComponent::lookAndFeelChanged() { repaint(); }

bool PlaylistComponent::isInterestedInFileDrag(const juce::StringArray &) {
  return true;
}
void PlaylistComponent::fileDragEnter(const juce::StringArray &, int, int) {
  isExternalFileDragging = true;
  repaint();
}
void PlaylistComponent::fileDragExit(const juce::StringArray &) {
  isExternalFileDragging = false;
  repaint();
}
void PlaylistComponent::filesDropped(const juce::StringArray &files, int x,
                                     int y) {
  isExternalFileDragging = false;
  PlaylistDropHandler::processExternalFiles(files, x, y, *this);
}

bool PlaylistComponent::isInterestedInDragSource(
    const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
  juce::String desc = dragSourceDetails.description.toString();
  return desc.startsWith("PickerDrag|") || desc == "FileBrowserDrag";
}
void PlaylistComponent::itemDragEnter(
    const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
  isInternalDragging = true;
  repaint();
}
void PlaylistComponent::itemDragExit(
    const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
  isInternalDragging = false;
  repaint();
}
void PlaylistComponent::itemDropped(
    const juce::DragAndDropTarget::SourceDetails &dragSourceDetails) {
  isInternalDragging = false;
  PlaylistDropHandler::processInternalItem(dragSourceDetails, *this);
}