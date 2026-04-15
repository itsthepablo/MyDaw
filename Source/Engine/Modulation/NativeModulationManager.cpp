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

                case ModTarget::EQ_B1_Freq: eq.applyModB1Freq(unipolar); break;
                case ModTarget::EQ_B1_Gain: eq.applyModB1Gain(unipolar); break;
                case ModTarget::EQ_B1_Q:    eq.applyModB1Q(unipolar); break;
                case ModTarget::EQ_B2_Freq: eq.applyModB2Freq(unipolar); break;
                case ModTarget::EQ_B2_Gain: eq.applyModB2Gain(unipolar); break;
                case ModTarget::EQ_B2_Q:    eq.applyModB2Q(unipolar); break;

                default: break;
            }
        }
    }

    // --- ACTUALIZACIÓN ATÓMICA FINAL (Solo una vez por bloque) ---
    track->mixerData.visSync.hasVol.store(foundVol);
    track->mixerData.visSync.hasPan.store(foundPan);
    track->gainStationData.visSync.hasPre.store(foundPre);
    track->gainStationData.visSync.hasPost.store(foundPost);

    // El EQ gestiona su propio reset de los parámetros no modulados
    eq.resetModulations();

    if (!foundVol) track->mixerData.modVolumeSmoother.setTargetValue(0.0f);
    if (!foundPan) track->mixerData.modPanSmoother.setTargetValue(0.0f);
}

// Legacy methods for compatibility
bool NativeModulationManager::hasModulation(Track* track, ModTarget::Type targetType) { return false; }
float NativeModulationManager::getAbsoluteModulatedValue(Track* track, ModTarget::Type targetType, double beatPhase, float min, float max, bool map) { return 0.0f; }
