#pragma once
#include <JuceHeader.h>
#include <map>

/**
 * BalanceTrackData — Historial de desviación L/R.
 */
struct BalanceTrackData {
    struct BalanceHistory {
        std::map<double, float> points; // samplePos -> balanceDB (-12 a +12)
        float referenceScaleDB = 12.0f; 
        bool isActive = true;

        void clear() { points.clear(); }
        void addPoint(double pos, float val) {
            auto it = points.lower_bound(pos);
            auto itEnd = points.upper_bound(pos + 2000.0); // Salto mayor para balance
            if (it != points.end()) {
                points.erase(it, itEnd);
            }
            points[pos] = val;
        }
    };

    BalanceHistory history;

    void clearAll() {
        history.clear();
    }
};
