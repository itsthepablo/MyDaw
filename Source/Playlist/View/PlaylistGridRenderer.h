#pragma once
#include <JuceHeader.h>
#include "../../Theme/CustomTheme.h"
#include "../../Data/Track.h"

class PlaylistGridRenderer {
public:
    /**
     * drawGrid
     * Se encarga de dibujar el fondo alternado y las líneas de la rejilla.
     * Incluye los separadores horizontales de pistas.
     */
    /**
     * drawVerticalGrid
     * Fondo alternado y líneas verticales. Independiente del scroll vertical.
     */
    static void drawVerticalGrid(juce::Graphics& g, 
                                 int topOffset, 
                                 int height, 
                                 int width, 
                                 double hS, 
                                 double hZoom, 
                                 double snapPixels, 
                                 double timelineLength,
                                 juce::LookAndFeel& lnf);

    /**
     * drawHorizontalSeparators
     * Líneas entre pistas. Sujeto al scroll vertical (vS).
     */
    static void drawHorizontalSeparators(juce::Graphics& g, 
                                         int topOffset, 
                                         int viewAreaH, 
                                         int width, 
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
                                  double hS, 
                                  double hZoom, 
                                  double timelineLength);

    /**
     * drawPlayhead
     * Dibuja el cabezal de reproducción con su estela y triángulo.
     */
    static void drawPlayhead(juce::Graphics& g, 
                             float playheadAbsPos, 
                             double hZoom, 
                             double hS, 
                             int menuBarH, 
                             int navigatorH, 
                             int width, 
                             int height);
};
