#pragma once
#include <JuceHeader.h>
#include "../../Theme/CustomTheme.h"
#include "../../Tracks/Track.h"

class PlaylistGridRenderer {
public:
    /**
     * drawGrid
     * Se encarga de dibujar el fondo alternado y las líneas de la rejilla.
     * Incluye los separadores horizontales de pistas.
     */
    static void drawGrid(juce::Graphics& g, 
                         int topOffset, 
                         int viewAreaH, 
                         int width, 
                         int height, 
                         float hS, 
                         float hZoom, 
                         double snapPixels, 
                         double timelineLength,
                         juce::LookAndFeel& lnf,
                         const juce::OwnedArray<Track>* tracksRef,
                         float vS,
                         float trackHeight);

    /**
     * drawTimelineRuler
     * Dibuja los números de compás y las marcas de tiempo en la parte superior.
     */
    static void drawTimelineRuler(juce::Graphics& g, 
                                  int menuBarH, 
                                  int navigatorH, 
                                  int timelineH, 
                                  int width, 
                                  float hS, 
                                  float hZoom, 
                                  double timelineLength);

    /**
     * drawPlayhead
     * Dibuja el cabezal de reproducción con su estela y triángulo.
     */
    static void drawPlayhead(juce::Graphics& g, 
                             float playheadAbsPos, 
                             float hZoom, 
                             float hS, 
                             int menuBarH, 
                             int navigatorH, 
                             int width, 
                             int height);
};
