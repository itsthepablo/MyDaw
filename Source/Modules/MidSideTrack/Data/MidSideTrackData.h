#pragma once
#include <JuceHeader.h>
#include <map>

/**
 * MidSideTrackData — Historial de energía Mid/Side y correlación.
 */
struct MidSideTrackData {
    struct MidSidePoint {
        float mid, side, correlation;
    };

    struct MidSideHistory {
        std::map<double, MidSidePoint> points; 
        float referenceScaleDB = 1.0f; 
        int displayMode = 0; // 0=Overlay, 1=Mid, 2=Side

        void clear() { points.clear(); }
        void addPoint(double pos, float m, float s, float c) {
            auto it = points.lower_bound(pos);
            auto itEnd = points.upper_bound(pos + 2000.0);
            if (it != points.end()) {
                points.erase(it, itEnd);
            }
            points[pos] = { m, s, c };
        }
    };

    MidSideHistory history;

    void clearAll() {
        history.clear();
    }
};
