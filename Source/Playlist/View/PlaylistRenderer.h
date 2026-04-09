#pragma once
#include <JuceHeader.h>
#include "../PlaylistComponent.h"
#include "../../Data/Track.h"
#include <unordered_map>

/**
 * PlaylistViewContext
 * Estructura para pasar todo el contexto de vista a los renderizadores 
 * sin saturar las firmas de los métodos.
 */
struct PlaylistViewContext {
    const juce::OwnedArray<Track>* tracksRef;
    const std::vector<TrackClip>* clips;
    float hZoom;
    float hS;
    float vS;
    float trackHeight;
    int topOffset;
    int width;
    int height;
    float playheadAbsPos;
    const std::vector<int>* selectedClipIndices;
    AutomationClipData* hoveredAutoClip;
    bool isExternalFileDragging;
    bool isInternalDragging;
};

class PlaylistRenderer {
public:
    static void drawTracksAndClips(juce::Graphics& g, 
                                   const PlaylistViewContext& ctx,
                                   std::unordered_map<Track*, int>& trackYCache);

    static void drawFolderSummaries(juce::Graphics& g, 
                                    const PlaylistViewContext& ctx,
                                    const std::unordered_map<Track*, int>& trackYCache);

    static void drawSpecialAnalysisTracks(juce::Graphics& g, 
                                          const PlaylistViewContext& ctx,
                                          const std::unordered_map<Track*, int>& trackYCache);

    static void drawAutomationOverlays(juce::Graphics& g, 
                                       const PlaylistViewContext& ctx,
                                       const std::unordered_map<Track*, int>& trackYCache);

    static void drawDragOverlays(juce::Graphics& g, 
                                 const PlaylistViewContext& ctx);

    static void drawMinimap(juce::Graphics& g, 
                            juce::Rectangle<int> bounds,
                            const juce::OwnedArray<Track>* tracksRef,
                            const std::vector<TrackClip>& clips,
                            double timelineLength);
};
