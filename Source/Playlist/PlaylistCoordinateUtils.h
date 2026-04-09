#pragma once
#include <JuceHeader.h>
#include "../Data/Track.h"
#include "PlaylistClip.h"
#include "ScrollBar/VerticalNavigator.h"
#include "ScrollBar/PlaylistNavigator.h"

class PlaylistCoordinateUtils {
public:
    /**
     * getTrackY
     * Retorna la coordenada Y en pantalla del track dado.
     * BUG FIX: Si el track está oculto por una carpeta, retorna -1.
     */
    static int getTrackY(Track* targetTrack, 
                         const juce::OwnedArray<Track>* tracksRef,
                         int topOffset,
                         const VerticalNavigator& vBar,
                         float trackHeight)
    {
        if (!tracksRef || !targetTrack)
            return -1;

        // BUG FIX: Si el track está oculto por una carpeta, sus clips NO TIENEN
        // COORDENADA Y (deberían ocultarse)
        if (!targetTrack->isShowingInChildren)
            return -1;

        int currentY = topOffset - (int)vBar.getCurrentRangeStart();

        for (auto *t : *tracksRef) {
            if (t == targetTrack)
                return currentY;
            if (t->isShowingInChildren)
                currentY += (int)trackHeight;
        }
        return -1;
    }

    /**
     * getTrackAtY
     * Retorna el índice del track en la posición Y dada, o -1 si no hay ninguno.
     */
    static int getTrackAtY(int y,
                           int topOffset,
                           const VerticalNavigator& vBar,
                           const juce::OwnedArray<Track>* tracksRef,
                           float trackHeight)
    {
        if (y < topOffset)
            return -1;

        int vS = (int)vBar.getCurrentRangeStart();
        int currentY = topOffset - vS;

        if (!tracksRef)
            return -1;
        for (int i = 0; i < (int)tracksRef->size(); ++i) {
            auto *t = (*tracksRef)[i];
            if (!t->isShowingInChildren)
                continue;
            if (y >= currentY && y < currentY + trackHeight)
                return i;
            currentY += (int)trackHeight;
        }
        return -1;
    }

    /**
     * getClipAt
     * Retorna el índice del clip en la posición (x, y) dada, o -1 si no hay ninguno.
     */
    static int getClipAt(int x, int y,
                         const PlaylistNavigator& hNavigator,
                         float hZoom,
                         int topOffset,
                         const VerticalNavigator& vBar,
                         const juce::OwnedArray<Track>* tracksRef,
                         float trackHeight,
                         const std::vector<TrackClip>& clips)
    {
        int tIdx = getTrackAtY(y, topOffset, vBar, tracksRef, trackHeight);
        if (tIdx == -1)
            return -1;
        float hS = (float)hNavigator.getCurrentRangeStart();
        float absX = (x + hS) / hZoom;
        for (int i = (int)clips.size() - 1; i >= 0; --i) {
            if (clips[i].trackPtr == (*tracksRef)[tIdx] && absX >= clips[i].startX &&
                absX <= clips[i].startX + clips[i].width)
                return i;
        }
        return -1;
    }
};
