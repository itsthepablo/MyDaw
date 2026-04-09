#pragma once
#include "../../Tracks/Track.h"

/**
 * MixerParameterBridge: Interfaz centralizada para la comunicación entre 
 * componentes (UI, Playlist, DSP) y los datos de mezcla de un Track.
 * 
 * Sigue el patrón "Bridge" para desacoplar la implementación interna de MixerChannelData
 * del resto del sistema.
 */
class MixerParameterBridge {
public:
    // --- VOLUMEN ---
    static float getVolume(const Track* t) {
        return t ? t->getVolume() : 1.0f;
    }
    
    static void setVolume(Track* t, float v) {
        if (t) t->setVolume(v);
    }

    // --- BALANCE / PAN ---
    static float getBalance(const Track* t) {
        return t ? t->getBalance() : 0.0f;
    }
    
    static void setBalance(Track* t, float b) {
        if (t) t->setBalance(b);
    }

    // --- MUTE ---
    static bool isMuted(const Track* t) {
        return t ? t->mixerData.isMuted.load(std::memory_order_relaxed) : false;
    }
    
    static void setMuted(Track* t, bool m) {
        if (t) t->mixerData.isMuted.store(m, std::memory_order_relaxed);
    }

    // --- SOLO ---
    static bool isSoloed(const Track* t) {
        return t ? t->mixerData.isSoloed.load(std::memory_order_relaxed) : false;
    }
    
    static void setSoloed(Track* t, bool s) {
        if (t) t->mixerData.isSoloed.store(s, std::memory_order_relaxed);
    }

    // --- FASE (POLARIDAD) ---
    static bool isPhaseInverted(const Track* t) {
        return t ? t->isPhaseInverted() : false;
    }
    
    static void setPhaseInverted(Track* t, bool i) {
        if (t) t->setPhaseInverted(i);
    }

    // --- METERING ---
    static float getPeakLevelL(const Track* t) {
        return t ? t->mixerData.currentPeakLevelL.load(std::memory_order_relaxed) : 0.0f;
    }
    
    static float getPeakLevelR(const Track* t) {
        return t ? t->mixerData.currentPeakLevelR.load(std::memory_order_relaxed) : 0.0f;
    }

    static void resetPeakLevels(Track* t) {
        if (t) {
            t->mixerData.currentPeakLevelL.store(0.0f, std::memory_order_relaxed);
            t->mixerData.currentPeakLevelR.store(0.0f, std::memory_order_relaxed);
        }
    }

    // --- PANNING MODE ---
    static bool isPanningModeDual(const Track* t) {
        return t ? t->mixerData.panningModeDual.load(std::memory_order_relaxed) : false;
    }

    static void setPanningModeDual(Track* t, bool dual) {
        if (t) t->mixerData.panningModeDual.store(dual, std::memory_order_relaxed);
    }

    // --- DUAL PAN ---
    static float getPanL(const Track* t) {
        return t ? t->mixerData.panL.load(std::memory_order_relaxed) : 0.0f;
    }

    static void setPanL(Track* t, float v) {
        if (t) t->mixerData.panL.store(v, std::memory_order_relaxed);
    }

    static float getPanR(const Track* t) {
        return t ? t->mixerData.panR.load(std::memory_order_relaxed) : 0.0f;
    }

    static void setPanR(Track* t, float v) {
        if (t) t->mixerData.panR.store(v, std::memory_order_relaxed);
    }

    // --- MONO (Si aplica) ---
    static bool isMonoActive(const Track* t) {
        return t ? t->isMonoActive() : false;
    }
    
    static void setMonoActive(Track* t, bool m) {
        if (t) t->setMonoActive(m);
    }
};
