#pragma once
#include <JuceHeader.h>
#include <map>

/**
 * LoudnessTrackData — Almacena exclusivamente el historial de LUFS.
 * Parte del módulo LoudnessTrack (Black Box).
 */
struct LoudnessTrackData {
    struct LoudnessHistory {
        std::map<double, float> points; // samplePos -> shortTermLUFS
        float referenceLUFS = -23.0f;
        bool isActive = true;

        void clear() { points.clear(); }
        void addPoint(double pos, float val) {
            auto it = points.lower_bound(pos);
            auto itEnd = points.upper_bound(pos + 500.0);
            if (it != points.end()) {
                points.erase(it, itEnd);
            }
            points[pos] = val;
        }
    };

    LoudnessHistory history;

    void clearAll() {
        history.clear();
    }
};
