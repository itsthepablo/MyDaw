#pragma once
#include <JuceHeader.h>
#include "../Bridges/MidSideTrackBridge.h"
#include <vector>

/**
 * MidSideTrackRenderer — Renderizador especializado para el modulo MidSideTrack.
 */
class MidSideTrackRenderer
{
public:
    struct MSRenderPoint { 
        float x, off; 
    };

    static void drawMidSideTrack(juce::Graphics& g, 
                                 MidSideTrackBridge* bridge,
                                 juce::Rectangle<int> area,
                                 double hZoom, 
                                 double hScroll)
    {
        if (!bridge) return;
        auto& points = bridge->getPoints();
        if (points.empty()) return;

        float centerY = (float)area.getCentreY();
        float bottomY = (float)area.getBottom() - 12.0f; 
        float vZoom = bridge->getReferenceScale();          
        float hHalf = (area.getHeight() - 28.0f) * 0.5f; 

        juce::Path midPath, sidePath;
        juce::Path corrPath;
        std::vector<MSRenderPoint> mPoints, sPoints;
        mPoints.reserve(512); sPoints.reserve(512);
        
        double lastSamplePos = -100.0;
        const double gapThreshold = 2000.0;

        auto buildAndDraw = [&](juce::Path& path, std::vector<MSRenderPoint>& pts, juce::Colour fillCol) {
            if (pts.empty()) return;
            path.startNewSubPath(pts[0].x, centerY - pts[0].off);
            for (size_t i = 1; i < pts.size(); ++i) path.lineTo(pts[i].x, centerY - pts[i].off);
            for (int i = (int)pts.size() - 1; i >= 0; --i) path.lineTo(pts[i].x, centerY + pts[i].off);
            path.closeSubPath();
            g.setColour(fillCol);
            g.fillPath(path);
        };

        int displayMode = bridge->getDisplayMode();
        bool drawMid = (displayMode == 0 || displayMode == 1);
        bool drawSide = (displayMode == 0 || displayMode == 2);

        for (const auto& pair : points) {
            double sPos = pair.first;
            const auto& val = pair.second;
            float x = (float)(sPos * hZoom) - (float)hScroll;
            if (x < -100.0f) { lastSamplePos = sPos; continue; }
            if (x > (float)area.getRight() + 100.0f) break;

            auto getOffset = [&](float amp) {
                float norm = juce::jlimit(0.0f, 1.0f, amp * vZoom);
                return norm * hHalf;
            };

            float offM = getOffset(val.mid);
            float offS = getOffset(val.side);
            bool isJump = (lastSamplePos >= 0) && (sPos - lastSamplePos) > gapThreshold;

            if (isJump) {
                if (drawMid) buildAndDraw(midPath, mPoints, juce::Colours::silver.withAlpha(0.25f));
                if (drawSide) buildAndDraw(sidePath, sPoints, juce::Colour(255, 215, 0).withAlpha(0.35f));
                midPath.clear(); sidePath.clear();
                mPoints.clear(); sPoints.clear();
            }

            mPoints.push_back({ x, offM });
            sPoints.push_back({ x, offS });

            float yC = bottomY - ((val.correlation + 1.0f) * 0.5f * 8.0f);
            if (corrPath.isEmpty() || isJump) corrPath.startNewSubPath(x, yC);
            else corrPath.lineTo(x, yC);
            lastSamplePos = sPos;
        }
        
        if (drawMid) buildAndDraw(midPath, mPoints, juce::Colours::silver.withAlpha(0.25f));
        if (drawSide) buildAndDraw(sidePath, sPoints, juce::Colour(255, 215, 0).withAlpha(0.35f));

        g.setColour(juce::Colours::greenyellow.withAlpha(0.7f));
        g.strokePath(corrPath, juce::PathStrokeType(1.0f));
    }
};
