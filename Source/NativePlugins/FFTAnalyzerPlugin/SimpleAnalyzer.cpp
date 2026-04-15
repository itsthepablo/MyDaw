#include "SimpleAnalyzer.h"

SimpleAnalyzer::SimpleAnalyzer() : fftCfg(nullptr)
{
    setup(12, 44100.0);
}

SimpleAnalyzer::~SimpleAnalyzer() 
{ 
    if (fftCfg) free(fftCfg); 
}

void SimpleAnalyzer::setup(int order, double sampleRate)
{
    const juce::ScopedLock sl(lock);

    fftOrder = order;
    fftSize = 1 << fftOrder;
    numBins = fftSize / 2 + 1;
    currentSampleRate = sampleRate;

    if (fftCfg) free(fftCfg);
    fftCfg = kiss_fft_alloc(fftSize, 0, nullptr, nullptr);

    circularBuffer.resize(fftSize * 2, 0.0f);
    processingBuffer.resize(fftSize, 0.0f);
    fftOut.resize(fftSize);

    visualSize = 800;

    avgData.assign(visualSize, -140.0f);
    peakData.assign(visualSize, -140.0f);
    spectrumData.assign(visualSize, -140.0f);

    displayAvg.assign(visualSize, -140.0f);
    displayPeak.assign(visualSize, -140.0f);
    displaySpec.assign(visualSize, -140.0f);
}

void SimpleAnalyzer::pushBuffer(const float* inputChannel, int numSamples)
{
    if (lock.tryEnter())
    {
        for (int i = 0; i < numSamples; ++i)
        {
            if (circularBuffer.size() > 0) {
                circularBuffer[writePos] = inputChannel[i];
                writePos = (writePos + 1) % circularBuffer.size();
            }
        }
        lock.exit();
    }
}

void SimpleAnalyzer::process()
{
    if (lock.tryEnter())
    {
        int readPos = writePos - fftSize;
        if (readPos < 0) readPos += circularBuffer.size();

        for (int i = 0; i < fftSize; ++i)
        {
            processingBuffer[i] = circularBuffer[(readPos + i) % circularBuffer.size()];
        }
        lock.exit();

        performFFT();
        updatePhysics();

        displayAvg = avgData;
        displayPeak = peakData;
        displaySpec = spectrumData;
    }
}

float SimpleAnalyzer::getExactMagnitude(float binIndex)
{
    int i = (int)binIndex;
    float frac = binIndex - i;
    if (i >= numBins - 1) return 0.0f;
    if (i < 0) return 0.0f;

    float mag1 = std::sqrt(fftOut[i].r * fftOut[i].r + fftOut[i].i * fftOut[i].i);
    float mag2 = std::sqrt(fftOut[i + 1].r * fftOut[i + 1].r + fftOut[i + 1].i * fftOut[i + 1].i);
    return mag1 * (1.0f - frac) + mag2 * frac;
}

void SimpleAnalyzer::performFFT()
{
    std::vector<kiss_fft_cpx> localTimeData(fftSize);
    for (int i = 0; i < fftSize; ++i)
    {
        float window = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * i / (fftSize - 1)));
        localTimeData[i].r = processingBuffer[i] * window;
        localTimeData[i].i = 0.0f;
    }

    if (fftCfg) kiss_fft(fftCfg, localTimeData.data(), fftOut.data());
}

void SimpleAnalyzer::updatePhysics()
{
    float minFreq = 20.0f;
    float maxFreq = 20000.0f;
    float logMin = std::log10(minFreq);
    float logMax = std::log10(maxFreq);

    float octaveWidthMaster = 0.666f;

    for (int i = 0; i < visualSize; ++i)
    {
        float normX = (float)i / (float)(visualSize - 1);
        float centerFreq = std::pow(10.0f, logMin + normX * (logMax - logMin));
        float centerBinFloat = centerFreq * fftSize / currentSampleRate;

        // ==========================================
        // 1. CÁLCULO MASTER (Con Roll-off estético)
        // ==========================================
        float freqFactorM = std::pow(2.0f, octaveWidthMaster * 0.5f);
        float bandwidthHzM = centerFreq * (freqFactorM - (1.0f / freqFactorM));
        float bandwidthBinsM = bandwidthHzM * fftSize / currentSampleRate;

        float sigmaM = std::max(2.5f, bandwidthBinsM / 2.2f);
        float rangeMultM = 4.0f;
        float weightedEnergySumM = 0.0f;
        float weightSumM = 0.0f;

        int startBinM = (int)std::floor(centerBinFloat - sigmaM * rangeMultM);
        int endBinM = (int)std::ceil(centerBinFloat + sigmaM * rangeMultM);
        if (startBinM < 1) startBinM = 1;
        if (endBinM >= numBins) endBinM = numBins - 1;
        if (startBinM > endBinM) startBinM = endBinM;

        for (int k = startBinM; k <= endBinM; ++k)
        {
            float dist = (float)k - centerBinFloat;
            float weight = std::exp(-0.5f * (dist * dist) / (sigmaM * sigmaM));
            float binEnergy = (fftOut[k].r * fftOut[k].r) + (fftOut[k].i * fftOut[k].i);
            weightedEnergySumM += binEnergy * weight;
            weightSumM += weight;
        }
        float avgEnergyM = (weightSumM > 0.0f) ? (weightedEnergySumM / weightSumM) : 0.0f;
        float magGaussM = std::sqrt(avgEnergyM) / (float)fftSize;

        magGaussM *= 1.2f;

        float dbGauss = -140.0f;
        if (magGaussM > 1e-12f) dbGauss = 20.0f * std::log10(magGaussM);
        if (centerFreq > 1.0f) dbGauss += 4.5f * std::log2(centerFreq / 1000.0f);

        float inputMaster = dbGauss + calibration;

        // Solo el Master cae en los extremos
        if (centerFreq < 45.0f) {
            float dropRatio = (45.0f - centerFreq) / 25.0f;
            inputMaster -= (dropRatio * dropRatio * 18.0f);
        }
        if (centerFreq > 10000.0f) {
            float dropRatio = (centerFreq - 10000.0f) / 10000.0f;
            inputMaster -= (dropRatio * dropRatio * 18.0f);
        }

        if (inputMaster > 0.0f) inputMaster = 0.0f;
        if (inputMaster < -140.0f) inputMaster = -140.0f;


        // ==========================================
        // 2. CÁLCULO DEFAULT (Lineal y Puro)
        // ==========================================
        float normX_prev = std::max(0.0f, (float)(i - 0.5f) / (float)(visualSize - 1));
        float normX_next = std::min(1.0f, (float)(i + 0.5f) / (float)(visualSize - 1));
        float freqPrev = std::pow(10.0f, logMin + normX_prev * (logMax - logMin));
        float freqNext = std::pow(10.0f, logMin + normX_next * (logMax - logMin));

        float binPrev = freqPrev * fftSize / currentSampleRate;
        float binNext = freqNext * fftSize / currentSampleRate;
        float magRaw = 0.0f;

        if ((binNext - binPrev) < 1.0f) {
            magRaw = getExactMagnitude(centerBinFloat) / (float)fftSize;
        }
        else {
            int bStart = (int)std::floor(binPrev);
            int bEnd = (int)std::ceil(binNext);
            if (bStart < 1) bStart = 1;
            if (bEnd >= numBins) bEnd = numBins - 1;

            float sumE = 0.0f;
            int count = 0;

            for (int k = bStart; k <= bEnd; ++k) {
                float e = (fftOut[k].r * fftOut[k].r) + (fftOut[k].i * fftOut[k].i);
                sumE += e;
                count++;
            }

            float avgE = (count > 0) ? (sumE / (float)count) : 0.0f;
            magRaw = std::sqrt(avgE) / (float)fftSize;
        }

        magRaw *= 1.2f;

        float dbDefault = -140.0f;
        if (magRaw > 1e-12f) dbDefault = 20.0f * std::log10(magRaw);
        if (centerFreq > 1.0f) dbDefault += 4.5f * std::log2(centerFreq / 1000.0f);

        float inputDefault = dbDefault + calibration;

        // En Default NO aplicamos Roll-off para mantenerlo como referencia plana
        if (inputDefault > 0.0f) inputDefault = 0.0f;
        if (inputDefault < -140.0f) inputDefault = -140.0f;


        // ==========================================
        // 3. BALÍSTICA MASTER
        // ==========================================
        float slowAttack = 0.1f;
        float slowRelease = 0.003f;
        if (inputMaster > avgData[i]) avgData[i] = avgData[i] * (1.0f - slowAttack) + inputMaster * slowAttack;
        else avgData[i] = avgData[i] * (1.0f - slowRelease) + inputMaster * slowRelease;

        float inputBright = inputMaster - 5.0f;
        float fastAttack = 0.05f;
        float fastRelease = 0.04f;
        if (inputBright > peakData[i]) peakData[i] = peakData[i] * (1.0f - fastAttack) + inputBright * fastAttack;
        else peakData[i] = peakData[i] * (1.0f - fastRelease) + inputBright * fastRelease;


        // ==========================================
        // 4. BALÍSTICA DEFAULT
        // ==========================================
        float eqAttack = 0.35f;
        float eqRelease = 0.02f;

        if (inputDefault > spectrumData[i]) spectrumData[i] = spectrumData[i] * (1.0f - eqAttack) + inputDefault * eqAttack;
        else spectrumData[i] = spectrumData[i] * (1.0f - eqRelease) + inputDefault * eqRelease;
    }
}
