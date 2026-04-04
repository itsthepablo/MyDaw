#pragma once
#include <JuceHeader.h>
#include "../../Tracks/Track.h"
#include <vector>

// ==============================================================================
// MidSideRenderer — Renderizador con capas de cebolla para análisis M/S.
// Muestra energía Mid (Centro) y Side (Laterales) superpuestas en modo espejo.
// Optimizado para visualización por PICOS y Amplitud Lineal (Estilo Vectorscopio).
// ==============================================================================
class MidSideRenderer
{
public:
    struct MSRenderPoint { 
        float x, off; 
    };

    static void drawMidSideTrack(juce::Graphics& g, 
                                 const MidSideHistory& history,
                                 juce::Rectangle<int> area,
                                 double hZoom, 
                                 double hScroll)
    {
        if (history.points.empty()) return;

        // 1. PREPARAR FONDO Y ZOOM
        float centerY = (float)area.getCentreY();
        float bottomY = (float)area.getBottom() - 12.0f; // Espacio para fase
        float vZoom = history.referenceScaleDB;          // Interpretado como Zoom (1x, 2x, etc)
        float hHalf = (area.getHeight() - 28.0f) * 0.5f; 

        // 2. PREPARAR PATHS (Bipolares / Espejados)
        juce::Path midPath, sidePath;
        juce::Path corrPath;
        
        std::vector<MSRenderPoint> mPoints, sPoints;
        mPoints.reserve(512); sPoints.reserve(512);
        
        double lastSamplePos = -100.0;
        const double gapThreshold = 2000.0;

        auto buildAndDraw = [&](juce::Path& path, std::vector<MSRenderPoint>& pts, juce::Colour fillCol, juce::Colour strokeCol, float strokeW) {
            if (pts.empty()) return;
            
            path.startNewSubPath(pts[0].x, centerY - pts[0].off);
            for (size_t i = 1; i < pts.size(); ++i) path.lineTo(pts[i].x, centerY - pts[i].off);
            for (int i = (int)pts.size() - 1; i >= 0; --i) path.lineTo(pts[i].x, centerY + pts[i].off);
            path.closeSubPath();

            g.setColour(fillCol);
            g.fillPath(path);
            g.setColour(strokeCol);
            g.strokePath(path, juce::PathStrokeType(strokeW));
        };

        bool drawMid = (history.displayMode == 0 || history.displayMode == 1);
        bool drawSide = (history.displayMode == 0 || history.displayMode == 2);

        for (const auto& pair : history.points) {
            double sPos = pair.first;
            const auto& val = pair.second;
            
            float x = (float)(sPos * hZoom) - (float)hScroll;
            if (x < -100.0f) { lastSamplePos = sPos; continue; }
            if (x > (float)area.getRight() + 100.0f) break;

            // Mapping: AMPLITUD LINEAL (Waveform style)
            // val.mid y val.side están en rango 0..1 (Picos)
            auto getOffset = [&](float amp) {
                float norm = juce::jlimit(0.0f, 1.0f, amp * vZoom);
                return norm * hHalf;
            };

            float offM = getOffset(val.mid);
            float offS = getOffset(val.side);
            
            bool isJump = (lastSamplePos >= 0) && (sPos - lastSamplePos) > gapThreshold;

            if (isJump) {
                if (drawMid) buildAndDraw(midPath, mPoints, juce::Colours::silver.withAlpha(0.25f), juce::Colours::white.withAlpha(0.4f), 0.7f);
                if (drawSide) buildAndDraw(sidePath, sPoints, juce::Colour(255, 215, 0).withAlpha(0.35f), juce::Colours::gold.withAlpha(0.5f), 0.9f);
                midPath.clear(); sidePath.clear();
                mPoints.clear(); sPoints.clear();
            }

            mPoints.push_back({ x, offM });
            sPoints.push_back({ x, offS });

            // Fase
            float yC = bottomY - ((val.correlation + 1.0f) * 0.5f * 8.0f);
            if (corrPath.isEmpty() || isJump) corrPath.startNewSubPath(x, yC);
            else corrPath.lineTo(x, yC);
            
            lastSamplePos = sPos;
        }
        
        if (drawMid) buildAndDraw(midPath, mPoints, juce::Colours::silver.withAlpha(0.25f), juce::Colours::white.withAlpha(0.4f), 0.7f);
        if (drawSide) buildAndDraw(sidePath, sPoints, juce::Colour(255, 215, 0).withAlpha(0.35f), juce::Colours::gold.withAlpha(0.5f), 0.9f);

        // 3. DIBUJAR CORRELACIÓN DE FASE
        g.setColour(juce::Colours::greenyellow.withAlpha(0.7f));
        g.strokePath(corrPath, juce::PathStrokeType(1.0f));
        
        // Etiquetas "M" y "S" compactas arriba a la izquierda
        g.setFont(juce::Font("Sans-Serif", 10.0f, juce::Font::bold));
        
        if (drawMid) {
            g.setColour(juce::Colours::white.withAlpha(0.4f));
            g.drawText("M", area.getX() + 5, area.getY() + 2, 12, 10, juce::Justification::left);
        }
        
        if (drawSide) {
            float xOffset = drawMid ? 18.0f : 5.0f;
            g.setColour(juce::Colours::gold.withAlpha(0.6f));
            g.drawText("S", (int)(area.getX() + xOffset), area.getY() + 2, 12, 10, juce::Justification::left);
        }
    }
};
