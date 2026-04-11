class VUBallistics
{
public:
    VUBallistics() 
    {
        setSampleRate(44100.0);
        calibrationDB.store(-18.0f);
    }

    void setSampleRate(double sr)
    {
        fs = sr;
        float tau = 0.3f / 4.6f; // Constante para ~300ms de integración
        alpha = 1.0f - std::exp(-1.0f / (tau * (float)fs));
        
        holdTimeSamples = (int)(fs * 2.0); // 2 segundos de Peak Hold
        peakDecay = std::pow(0.1f, 1.0f / (float)fs);
    }

    void setCalibration(float db) { calibrationDB.store(db); }

    void processStereo(const float* left, const float* right, int numSamples)
    {
        if (numSamples <= 0) return;

        for (int i = 0; i < numSamples; ++i)
        {
            // Canal L
            float absL = std::abs(left ? left[i] : 0.0f);
            levelL = levelL + (absL - levelL) * alpha;
            updatePeak(levelL, peakHoldL, holdCounterL);

            // Canal R
            float absR = std::abs(right ? right[i] : (left ? left[i] : 0.0f));
            levelR = levelR + (absR - levelR) * alpha;
            updatePeak(levelR, peakHoldR, holdCounterR);
        }

        // Suavizado final para la aguja de la UI (eliminar jitter)
        float smoothing = 0.15f; 
        smoothLevelL = smoothLevelL + (levelL - smoothLevelL) * smoothing;
        smoothLevelR = smoothLevelR + (levelR - smoothLevelR) * smoothing;
    }

    float getVuValueL() const { return getDB(smoothLevelL); }
    float getVuValueR() const { return getDB(smoothLevelR); }
    float getPeakHoldL() const { return getDB(peakHoldL); }
    float getPeakHoldR() const { return getDB(peakHoldR); }

private:
    double fs = 44100.0;
    float alpha = 0.0f;
    float levelL = 0.0f, levelR = 0.0f;
    float smoothLevelL = 0.0f, smoothLevelR = 0.0f;

    float peakHoldL = 0.0f, peakHoldR = 0.0f;
    int holdTimeSamples = 0;
    int holdCounterL = 0, holdCounterR = 0;
    float peakDecay = 0.999f;

    std::atomic<float> calibrationDB { -18.0f };

    void updatePeak(float current, float& hold, int& counter)
    {
        if (current >= hold) {
            hold = current;
            counter = holdTimeSamples;
        } else {
            if (counter > 0) counter--;
            else hold *= peakDecay;
        }
    }

    float getDB(float linear) const
    {
        float dbfs = (linear > 0.00001f) ? 20.0f * std::log10(linear) : -100.0f;
        return dbfs - calibrationDB.load(); // Ajustar por calibración (0 VU = ref)
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VUBallistics)
};
