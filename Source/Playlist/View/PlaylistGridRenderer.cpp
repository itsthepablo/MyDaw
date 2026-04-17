#include "PlaylistGridRenderer.h"
#include <cmath>
#include <algorithm>

void PlaylistGridRenderer::drawVerticalGrid(juce::Graphics& g, 
                                             int topOffset, 
                                             int height, 
                                             int width, 
                                             double hS, 
                                             double hZoom, 
                                             double snapPixels, 
                                             double timelineLength,
                                             juce::LookAndFeel& lnf)
{
    double blockLengthPx = 1280.0; // 4 compases * 320px
    double startTime = (double)hS / hZoom;
    double endTime = (double)(hS + width) / hZoom;

    int startBlock = (int)(startTime / blockLengthPx);
    int endBlock = (int)(endTime / blockLengthPx);

    for (int b = startBlock; b <= endBlock; ++b) {
        if (b % 2 != 0) {
            double blockAbsX = b * blockLengthPx;
            int xPos = (int)(blockAbsX * hZoom) - (int)hS;
            int wPos = (int)(std::ceil(blockLengthPx * hZoom));

            if (auto* theme = dynamic_cast<CustomTheme*>(&lnf)) {
                g.setColour(theme->getSkinColor("PLAYLIST_BG_ALT", juce::Colour(32, 34, 38)));
            } else {
                g.setColour(juce::Colour(32, 34, 38));
            }
            // Pintar todo el alto hasta el final del componente
            g.fillRect(xPos, topOffset, wPos, height - topOffset);
        }
    }

    double visualSnap = (snapPixels < 10.0) ? 80.0 : snapPixels;
    double currentDrawSnap = visualSnap;

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
    double endLineSearch = std::min(timelineLength, endTime);

    for (double i = startLineSearch; i <= endLineSearch; i += currentDrawSnap) {
        int dx = (int)(i * hZoom - hS);
        g.setColour(juce::Colours::black);
        g.drawVerticalLine(dx, (float)topOffset, (float)height);
    }
}

void PlaylistGridRenderer::drawHorizontalSeparators(juce::Graphics& g, 
                                                   int topOffset, 
                                                   int viewAreaH, 
                                                   int width, 
                                                   const juce::OwnedArray<Track>* tracksRef,
                                                   float vS,
                                                   float trackHeight)
{
    int currentY = topOffset - (int)vS;
    if (tracksRef) {
        for (auto *t : *tracksRef) {
            if (!t->isShowingInChildren)
                continue;
            
            g.setColour(juce::Colours::black); 
            g.fillRect(0.0f, (float)(currentY + (int)trackHeight - 2),
                       (float)width, 2.0f); 
            currentY += (int)trackHeight;
        }
    }
}

void PlaylistGridRenderer::drawTimelineRuler(juce::Graphics& g, 
                                              int menuBarH, 
                                              int navigatorH, 
                                              int timelineH, 
                                              int width, 
                                              double hS, 
                                              double hZoom, 
                                              double timelineLength)
{
    g.setColour(juce::Colour(20, 22, 25));
    g.fillRect(0, menuBarH + navigatorH, width, timelineH);

    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.setFont(12.0f);

    double pixelsPerMeasure = 320.0 * hZoom;
    int measureMod = 1;
    if (pixelsPerMeasure < 15.0)
        measureMod = 16;
    else if (pixelsPerMeasure < 30.0)
        measureMod = 8;
    else if (pixelsPerMeasure < 60.0)
        measureMod = 4;
    else if (pixelsPerMeasure < 120.0)
        measureMod = 2;

    for (double i = 0; i <= timelineLength; i += 80.0) {
        int dx = (int)(i * hZoom - hS);
        if (dx < -100 || dx > width + 100)
            continue;

        if (std::fmod(i, 320.0) == 0.0) {
            int measureNumber = (int)(i / 320.0) + 1;
            g.drawVerticalLine(dx, (float)(menuBarH + navigatorH + timelineH - 6),
                               (float)(menuBarH + navigatorH + timelineH));

            if (measureMod == 1 || (measureNumber - 1) % measureMod == 0) {
                g.drawText(juce::String(measureNumber), dx - 20, menuBarH + navigatorH,
                           40, timelineH, juce::Justification::centred, false);
            }
        } else {
            if (pixelsPerMeasure >= 30.0) {
                g.drawVerticalLine(dx, (float)(menuBarH + navigatorH + timelineH - 4),
                                   (float)(menuBarH + navigatorH + timelineH));
            }
        }
    }
}

void PlaylistGridRenderer::drawPlayhead(juce::Graphics& g, 
                                        float playheadAbsPos, 
                                        double hZoom, 
                                        double hS, 
                                        int menuBarH, 
                                        int navigatorH, 
                                        int width, 
                                        int height)
{
    int phX = (int)((double)playheadAbsPos * hZoom - hS);
    if (phX >= 0 && phX <= width) {
        float phTop = (float)(menuBarH + navigatorH);
        float phBottom = (float)height;

        // 1. Estela (Cola difuminada)
        int tailWidth = 40;
        juce::Rectangle<float> tailRect(phX - tailWidth, phTop, tailWidth, phBottom - phTop);
        juce::ColourGradient tailGrad(juce::Colours::transparentWhite, phX - tailWidth, 0.0f,
                                      juce::Colours::white.withAlpha(0.15f), phX, 0.0f, false);
        g.setGradientFill(tailGrad);
        g.fillRect(tailRect);

        // 2. Línea Blanca
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.drawVerticalLine(phX, phTop, phBottom);

        // 3. Triángulo
        juce::Path triangle;
        float triTopY = phTop;
        float triHeight = 8.0f;
        float triHalfWidth = 6.0f;
        triangle.addTriangle(phX - triHalfWidth, triTopY, phX + triHalfWidth, triTopY, phX, triTopY + triHeight);
        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.fillPath(triangle);
    }
}
