#include "NativeModulationManager.h"

void NativeModulationManager::applyModulations(Track* track, double beatPhase) {
    if (track == nullptr) return;

    auto& eq = track->dsp.inlineEQ;

    // --- DETECCIÓN ESTABLE (Evita el parpadeo visual entre bloques) ---
    bool foundVol = false, foundPan = false;
    bool foundPre = false, foundPost = false;
    bool fEQ[6] = {false, false, false, false, false, false};

    // --- PROCESAMIENTO MULTI-MODULADOR ---
    for (auto* m : track->modulators) {
        if (m == nullptr) continue;
        
        const juce::ScopedLock sl(m->targetsLock);
        float lfoVal = m->getValueAt(beatPhase);
        float unipolar = (lfoVal + 1.0f) * 0.5f;
        if (!std::isfinite(unipolar)) unipolar = 0.5f;

        for (const auto& target : m->targets) {
            float absVal = 0.0f;
            
            switch (target.type) {
                case ModTarget::Volume: 
                    absVal = juce::jmap(unipolar, 0.0f, 1.0f, 0.0f, 2.0f);
                    track->mixerData.modVolumeSmoother.setTargetValue(absVal - track->mixerData.volume);
                    track->mixerData.visSync.vol.store(absVal);
                    foundVol = true;
                    break;

                case ModTarget::Pan:
                    absVal = juce::jmap(unipolar, 0.0f, 1.0f, -1.0f, 1.0f);
                    track->mixerData.modPanSmoother.setTargetValue(absVal - track->mixerData.balance);
                    track->mixerData.visSync.pan.store(absVal);
                    foundPan = true;
                    break;

                case ModTarget::PreGain:
                    absVal = juce::jmap(unipolar, 0.0f, 1.0f, 0.0f, 4.0f);
                    track->gainStationData.modPreGainSmoother.setTargetValue(absVal - track->gainStationData.preGain);
                    track->gainStationData.visSync.pre.store(absVal);
                    foundPre = true;
                    break;

                case ModTarget::PostGain:
                    absVal = juce::jmap(unipolar, 0.0f, 1.0f, 0.0f, 4.0f);
                    track->gainStationData.modPostGainSmoother.setTargetValue(absVal - track->gainStationData.postGain);
                    track->gainStationData.visSync.post.store(absVal);
                    foundPost = true;
                    break;

                case ModTarget::EQ_B1_Freq:
                case ModTarget::EQ_B2_Freq: {
                    absVal = 20.0f * std::pow(20000.0f / 20.0f, unipolar);
                    bool b1 = (target.type == ModTarget::EQ_B1_Freq);
                    if (b1) { eq.modB1Freq.setTargetValue(absVal); eq.visSync.b1Freq.store(absVal); fEQ[0] = true; }
                    else    { eq.modB2Freq.setTargetValue(absVal); eq.visSync.b2Freq.store(absVal); fEQ[3] = true; }
                    break;
                }

                case ModTarget::EQ_B1_Gain:
                case ModTarget::EQ_B2_Gain: {
                    absVal = juce::jmap(unipolar, 0.0f, 1.0f, -24.0f, 24.0f);
                    bool b1 = (target.type == ModTarget::EQ_B1_Gain);
                    if (b1) { eq.modB1Gain.setTargetValue(absVal); eq.visSync.b1Gain.store(absVal); fEQ[1] = true; }
                    else    { eq.modB2Gain.setTargetValue(absVal); eq.visSync.b2Gain.store(absVal); fEQ[4] = true; }
                    break;
                }

                case ModTarget::EQ_B1_Q:
                case ModTarget::EQ_B2_Q: {
                    absVal = juce::jmap(unipolar, 0.0f, 1.0f, 0.1f, 10.0f);
                    bool b1 = (target.type == ModTarget::EQ_B1_Q);
                    if (b1) { eq.modB1Q.setTargetValue(absVal); eq.visSync.b1Q.store(absVal); fEQ[2] = true; }
                    else    { eq.modB2Q.setTargetValue(absVal); eq.visSync.b2Q.store(absVal); fEQ[5] = true; }
                    break;
                }
                default: break;
            }
        }
    }

    // --- ACTUALIZACIÓN ATÓMICA FINAL (Solo una vez por bloque) ---
    track->mixerData.visSync.hasVol.store(foundVol);
    track->mixerData.visSync.hasPan.store(foundPan);
    track->gainStationData.visSync.hasPre.store(foundPre);
    track->gainStationData.visSync.hasPost.store(foundPost);

    eq.visSync.hasB1Freq.store(fEQ[0]); eq.visSync.hasB1Gain.store(fEQ[1]); eq.visSync.hasB1Q.store(fEQ[2]);
    eq.visSync.hasB2Freq.store(fEQ[3]); eq.visSync.hasB2Gain.store(fEQ[4]); eq.visSync.hasB2Q.store(fEQ[5]);

    if (!foundVol) track->mixerData.modVolumeSmoother.setTargetValue(0.0f);
    if (!foundPan) track->mixerData.modPanSmoother.setTargetValue(0.0f);

    if (!fEQ[0]) eq.modB1Freq.setTargetValue(eq.b1Freq);
    if (!fEQ[1]) eq.modB1Gain.setTargetValue(eq.b1Gain);
    if (!fEQ[2]) eq.modB1Q.setTargetValue(eq.b1Q);
    if (!fEQ[3]) eq.modB2Freq.setTargetValue(eq.b2Freq);
    if (!fEQ[4]) eq.modB2Gain.setTargetValue(eq.b2Gain);
    if (!fEQ[5]) eq.modB2Q.setTargetValue(eq.b2Q);
}

// Legacy methods for compatibility
bool NativeModulationManager::hasModulation(Track* track, ModTarget::Type targetType) { return false; }
float NativeModulationManager::getAbsoluteModulatedValue(Track* track, ModTarget::Type targetType, double beatPhase, float min, float max, bool map) { return 0.0f; }
