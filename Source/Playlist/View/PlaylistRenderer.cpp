#include "PlaylistRenderer.h"
#include "../../UI/AutomationRenderer.h"
#include "../../Clips/Midi/UI/MidiPatternRenderer.h"
#include "../../UI/MidiPatternStyles.h"
#include "../../Clips/Audio/UI/AudioClipRenderer.h"
#include "../../Modules/LoudnessTrack/Bridges/LoudnessTrackBridge.h"
#include "../../Modules/LoudnessTrack/UI/LoudnessTrackRenderer.h"
#include "../../Modules/BalanceTrack/Bridges/BalanceTrackBridge.h"
#include "../../Modules/BalanceTrack/UI/BalanceTrackRenderer.h"
#include "../../Modules/MidSideTrack/Bridges/MidSideTrackBridge.h"
#include "../../Modules/MidSideTrack/UI/MidSideTrackRenderer.h"

void PlaylistRenderer::drawFolderSummaries(juce::Graphics& g, 
                                           const PlaylistViewContext& ctx,
                                           const std::unordered_map<Track*, int>& trackYCache)
{
    // --- NUEVO: RENDERIZADO DE RESÚMENES DE CARPETA (REAPER STYLE) ---
    if (!ctx.tracksRef) return;

    for (auto *t : *ctx.tracksRef) {
        if (t->getType() == TrackType::Folder) {
            int yPos = -1;
            if (trackYCache.count(t))
                yPos = trackYCache.at(t);
            if (yPos == -1)
                continue;

            // Culling vertical básico para el resumen de la carpeta
            if (yPos + (int)ctx.trackHeight < ctx.topOffset || yPos > ctx.height)
                continue;

            // 1. Recolectar IDs de todos los descendientes (recursivo)
            std::vector<int> descendantIds;
            std::function<void(int)> gatherDescendantIds = [&](int pid) {
                for (auto *trk : *ctx.tracksRef) {
                    if (trk->parentId == pid) {
                        descendantIds.push_back(trk->getId());
                        if (trk->getType() == TrackType::Folder)
                            gatherDescendantIds(trk->getId());
                    }
                }
            };
            gatherDescendantIds(t->getId());

            if (descendantIds.empty())
                continue;

            // 1. Calcular rango global de notas para la carpeta (Alineación
            // Vertical)
            int folderMinP = 127, folderMaxP = 0;
            bool folderHasMidi = false;
            for (const auto &clip : *ctx.clips) {
                if (clip.linkedMidi == nullptr)
                    continue;
                if (std::find(descendantIds.begin(), descendantIds.end(),
                              clip.trackPtr->getId()) != descendantIds.end()) {
                    folderHasMidi = true;
                    for (const auto &n : clip.linkedMidi->getNotes()) {
                        folderMinP = std::min(folderMinP, n.pitch);
                        folderMaxP = std::max(folderMaxP, n.pitch);
                    }
                }
            }
            if (folderHasMidi) {
                folderMinP = std::max(0, folderMinP - 2);
                folderMaxP = std::min(127, folderMaxP + 2);
                if (folderMaxP - folderMinP < 12)
                    folderMaxP = std::min(127, folderMinP + 12);
            }

            // 2. Dibujar resúmenes de clips que pertenezcan a esos descendientes
            juce::Rectangle<int> summaryArea(0, yPos + 20, ctx.width,
                                             (int)ctx.trackHeight - 24);
            for (const auto &clip : *ctx.clips) {
                if (clip.linkedAudio == nullptr && clip.linkedMidi == nullptr)
                    continue;

                bool isChild =
                    std::find(descendantIds.begin(), descendantIds.end(),
                              clip.trackPtr->getId()) != descendantIds.end();
                if (isChild) {
                    int xPos = (int)(clip.startX * ctx.hZoom - (double)ctx.hS);
                    int wPos = (int)(clip.width * ctx.hZoom);

                    // Solo renderizar si el clip es visible horizontalmente
                    if (xPos + wPos > 0 && xPos < ctx.width) {
                        juce::Rectangle<int> clipSummaryRect(
                            xPos, summaryArea.getY(), wPos, summaryArea.getHeight());

                        if (clip.linkedAudio != nullptr) {
                            double clipUnits = (double)clip.startX;
                            double viewUnits = (double)ctx.hS / ctx.hZoom;
                            AudioClipRenderer::drawWaveformSummary(
                                g, *clip.linkedAudio, clipSummaryRect.toFloat(), ctx.hZoom,
                                clipUnits, viewUnits);
                        } else if (clip.linkedMidi != nullptr) {
                            MidiPatternRenderer::drawMidiSummary(
                                g, *clip.linkedMidi, clipSummaryRect, (float)ctx.hZoom,
                                (float)ctx.hS, folderMinP, folderMaxP);
                        }
                    }
                }
            }
        }
    }
}

void PlaylistRenderer::drawTracksAndClips(juce::Graphics& g, 
                                          const PlaylistViewContext& ctx,
                                          std::unordered_map<Track*, int>& trackYCache)
{
    // --- OPTIMIZACIÓN DE RENDIMIENTO: CACHÉ DE COORDENADAS Y CULLING ---
    // Pre-calculamos las alturas Y de todas las pistas para evitar 5,000+
    // búsquedas redundantes (ya viene precalculado en trackYCache desde paint())

    // 2. Límites visibles para Culling Horizontal inteligente
    double visibleTimeStart = (double)ctx.hS / ctx.hZoom;
    double visibleTimeEnd = (double)(ctx.hS + ctx.width) / ctx.hZoom;

    for (int i = 0; i < (int)ctx.clips->size(); ++i) {
        const auto &clip = (*ctx.clips)[i];

        // --- CULLING HORIZONTAL REAL ---
        if (clip.startX + clip.width < visibleTimeStart ||
            clip.startX > visibleTimeEnd)
            continue;

        int yPos = -1;
        if (trackYCache.count(clip.trackPtr))
            yPos = trackYCache[clip.trackPtr];

        // BUG FIX: Si el track está oculto por una carpeta, yPos será -1. Ignoramos
        // este clip.
        if (yPos == -1)
            continue;

        int clipTop = yPos;
        int clipBottom = yPos + (int)ctx.trackHeight;

        // Culling vertical correcto: omitimos si el clip completo está por encima o
        // por debajo de la ventana
        if (clipBottom < ctx.topOffset || clipTop > ctx.height)
            continue;

        // BLINDAJE DE COORDENADAS (Anti-Overflow): Usamos las coordenadas reales sin recorte para mantener el anclaje
        float exactXPos = (float)(clip.startX * ctx.hZoom - (double)ctx.hS);
        float exactWPos = (float)(clip.width * ctx.hZoom);
        juce::Rectangle<float> clipRectF(exactXPos, (float)(yPos + 2),
                                         exactWPos - 1.0f,
                                         (float)((int)ctx.trackHeight - 4));

        // NO USAMOS INTERSECTION: El renderizador necesita la X original para calcular las muestras correctamente
        if (clipRectF.getRight() < 0 || clipRectF.getX() > ctx.width)
            continue;

        juce::Colour trackColor = clip.trackPtr->getColor();

        bool isMutedLocally = (clip.linkedAudio && clip.linkedAudio->getIsMuted()) ||
                              (clip.linkedMidi && clip.linkedMidi->getIsMuted());
        float alphaMult = isMutedLocally ? 0.3f : 1.0f;
        g.setOpacity(alphaMult);

        // 1. Dibujar el Clip
        // 1. Dibujar el Clip
        if (clip.linkedAudio != nullptr) {
            double clipUnits = (double)clip.startX;
            double viewUnits = (double)ctx.hS / ctx.hZoom;
            AudioClipRenderer::drawAudioClip(
                g, *clip.linkedAudio, clipRectF, trackColor,
                clip.trackPtr->getWaveformViewMode(), ctx.hZoom, clipUnits, viewUnits);
        }
        else if (clip.linkedMidi != nullptr) {
            MidiPatternRenderer::drawMidiPattern(
                g, *clip.linkedMidi, clipRectF.toNearestInt(), trackColor, clip.name,
                clip.trackPtr->isInlineEditingActive, ctx.hZoom, (int)ctx.hS, (double)ctx.playheadAbsPos);
        }

        // 4. Indicador de selección (Universal)
        if (std::find(ctx.selectedClipIndices->begin(), ctx.selectedClipIndices->end(), i) !=
            ctx.selectedClipIndices->end()) {
            g.setColour(juce::Colours::yellow);
            g.drawRoundedRectangle(clipRectF, 5.0f, 1.5f);
        }

        g.setOpacity(1.0f);
    }
}

void PlaylistRenderer::drawSpecialAnalysisTracks(juce::Graphics& g, 
                                                 const PlaylistViewContext& ctx,
                                                 const std::unordered_map<Track*, int>& trackYCache)
{
    // --- NUEVO: RENDERIZADO DE PISTAS DE LOUDNESS ---
    if (!ctx.tracksRef) return;

    for (auto* t : *ctx.tracksRef) {
        int yPos = trackYCache.count(t) ? trackYCache.at(t) : -1;
        if (yPos == -1) continue;

        if (t->getType() == TrackType::Loudness) {
            juce::Rectangle<float> loudnessArea(0.0f, (float)(yPos + 2), (float)ctx.width, (float)(ctx.trackHeight - 4));
            LoudnessTrackBridge bridge(t->loudnessTrackData);
            LoudnessTrackRenderer::drawLoudnessTrack(g, loudnessArea, &bridge, (float)ctx.hZoom, (float)ctx.hS);
        } else if (t->getType() == TrackType::Balance) {
            juce::Rectangle<int> balanceArea(0, yPos + 2, ctx.width, (int)ctx.trackHeight - 4);
            BalanceTrackBridge bridge(t->balanceTrackData);
            BalanceTrackRenderer::drawBalanceTrack(g, &bridge, balanceArea, ctx.hZoom, (double)ctx.hS);
        } else if (t->getType() == TrackType::MidSide) {
            juce::Rectangle<int> midSideArea(0, yPos + 2, ctx.width, (int)ctx.trackHeight - 4);
            MidSideTrackBridge bridge(t->midSideTrackData);
            MidSideTrackRenderer::drawMidSideTrack(g, &bridge, midSideArea, ctx.hZoom, (double)ctx.hS);
        }
    }
}

void PlaylistRenderer::drawAutomationOverlays(juce::Graphics& g, 
                                              const PlaylistViewContext& ctx,
                                              const std::unordered_map<Track*, int>& trackYCache)
{
    // --- NUEVO: RENDERIZADO DE AUTOMATION OVERLAYS (ABLETON STYLE) ---
    if (!ctx.tracksRef) return;

    for (auto *t : *ctx.tracksRef) {
        if (!t->isShowingInChildren)
            continue;

        for (auto *aut : t->automationClips) {
            if (aut && aut->isShowing) {
                int yPos = trackYCache.count(t) ? trackYCache.at(t) : -1;
                if (yPos == -1)
                    continue;
                if (yPos + (int)ctx.trackHeight < ctx.topOffset || yPos > ctx.height)
                    continue;

                juce::Rectangle<int> laneRect(0, yPos, ctx.width, (int)ctx.trackHeight);
                AutomationRenderer::drawAutomationLane(
                    g, laneRect, aut, ctx.hZoom, (float)ctx.hS, ctx.hoveredAutoClip == aut);
            }
        }
    }
}

void PlaylistRenderer::drawDragOverlays(juce::Graphics& g, const PlaylistViewContext& ctx)
{
    if (ctx.isExternalFileDragging || ctx.isInternalDragging) {
        g.fillAll(juce::Colours::dodgerblue.withAlpha(0.2f));
        g.setColour(juce::Colours::white);
        juce::String overlayText =
            ctx.isInternalDragging ? "SUELTA EL CLIP AQUI" : "SUELTA TU AUDIO AQUI";
        g.drawText(overlayText, 0, 0, ctx.width, ctx.height, juce::Justification::centred);
    }
}

void PlaylistRenderer::drawMinimap(juce::Graphics& g, 
                                   juce::Rectangle<int> bounds,
                                   const juce::OwnedArray<Track>* tracksRef,
                                   const std::vector<TrackClip>& clips,
                                   double timelineLength)
{
    if (!tracksRef || tracksRef->isEmpty())
        return;

    double maxTime = timelineLength;
    double scaleX = bounds.getWidth() / maxTime;

    int visibleTracks = 0;
    for (auto *t : *tracksRef) {
        if (t->isShowingInChildren)
            visibleTracks++;
    }
    if (visibleTracks == 0)
        visibleTracks = 1;

    float trackMinimapH =
        std::min(6.0f, (float)bounds.getHeight() / visibleTracks);
    float totalOccupiedH = trackMinimapH * visibleTracks;
    float startY = bounds.getY() + (bounds.getHeight() - totalOccupiedH) / 2.0f;

    for (const auto &clip : clips) {
        int tIdx = -1;
        int currentVisIdx = 0;
        for (int i = 0; i < tracksRef->size(); ++i) {
            auto *t = (*tracksRef)[i];
            if (t == clip.trackPtr) {
                if (!t->isShowingInChildren) {
                    tIdx = -1;
                    break;
                }
                tIdx = currentVisIdx;
                break;
            }
            if (t->isShowingInChildren)
                currentVisIdx++;
        }
        if (tIdx == -1)
            continue;

        float x = bounds.getX() + (clip.startX * scaleX);
        float w = clip.width * scaleX;
        float y = startY + (tIdx * trackMinimapH);

        g.setColour(clip.trackPtr->getColor().withAlpha(0.8f));
        float padY = trackMinimapH > 3.0f ? 1.0f : 0.0f;
        g.fillRoundedRectangle(x, y + padY, juce::jmax(1.0f, w),
                               juce::jmax(1.0f, trackMinimapH - (padY * 2.0f)),
                               1.5f);
    }
}
