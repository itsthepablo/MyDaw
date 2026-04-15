#pragma once
#include <JuceHeader.h>
#include <vector>
#include <complex>
#include <cmath>
#include <algorithm>
#include "kiss_fft.h"

class SimpleAnalyzer
{
public:
    SimpleAnalyzer();
    ~SimpleAnalyzer();

    void setup(int order, double sampleRate);
    void pushBuffer(const float* inputChannel, int numSamples);
    void process();

    const std::vector<float>& getSpectrumData() { return masterMode ? displayAvg : displaySpec; }
    const std::vector<float>& getPeakData() { return displayPeak; }

    void setMasterMode(bool isMaster) { masterMode = isMaster; }
    void setCalibration(float dbOffset) { calibration = dbOffset; }
    void setGreenAlgo(bool isMax) { isGreenMax = isMax; }
    bool isMasterMode() const { return masterMode; }

private:
    juce::CriticalSection lock;

    int fftOrder = 12;
    int fftSize = 4096;
    int numBins = 2049;
    double currentSampleRate = 44100.0;
    int visualSize = 800;

    bool masterMode = false;
    bool isGreenMax = true;
    float calibration = 16.0f;

    std::vector<float> circularBuffer;
    std::vector<float> processingBuffer;
    int writePos = 0;

    kiss_fft_cfg fftCfg;
    std::vector<kiss_fft_cpx> fftOut;

    std::vector<float> avgData;
    std::vector<float> peakData;
    std::vector<float> spectrumData;

    std::vector<float> displayAvg;
    std::vector<float> displayPeak;
    std::vector<float> displaySpec;

    float getExactMagnitude(float binIndex);
    void performFFT();
    void updatePhysics();
};