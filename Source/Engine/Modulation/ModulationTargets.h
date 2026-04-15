#pragma once
#include <JuceHeader.h>

/**
 * ModTarget: Estructura unificada para identificar cualquier parámetro modulable en el DAW.
 * Expandido para soportar un sistema de modulación universal nativo.
 */
struct ModTarget {
    enum Type { 
        None = 0, 
        
        // --- Mixer ---
        Volume, 
        Pan, 
        PanL,
        PanR,

        // --- GainStation ---
        PreGain, 
        PostGain, 
        
        // --- EQ (Banda 1) ---
        EQ_B1_Freq,
        EQ_B1_Gain,
        EQ_B1_Q,
        EQ_B1_Type,

        // --- EQ (Banda 2) ---
        EQ_B2_Freq,
        EQ_B2_Gain,
        EQ_B2_Q,
        EQ_B2_Type,

        // --- Plugin Externo ---
        PluginParam
    };

    Type type = None;
    int pluginIdx = -1;    
    int parameterIdx = -1; 
    
    juce::int64 getUid() const {
        if (type == None) return -1;
        return (juce::int64)type | ((juce::int64)pluginIdx << 16) | ((juce::int64)parameterIdx << 32);
    }

    bool operator==(const ModTarget& other) const {
        return type == other.type && pluginIdx == other.pluginIdx && parameterIdx == other.parameterIdx;
    }

    bool operator!=(const ModTarget& other) const {
        return !(*this == other);
    }

    bool isInternal() const {
        return type != None && type != PluginParam;
    }

    bool isValid() const {
        return type != None;
    }

    static juce::String getName(Type type) {
        switch (type) {
            case Volume:    return "Volume";
            case Pan:       return "Pan";
            case PanL:      return "Pan L";
            case PanR:      return "Pan R";
            case PreGain:   return "Pre-Gain";
            case PostGain:  return "Post-Gain";
            case EQ_B1_Freq: return "EQ B1 Freq";
            case EQ_B1_Gain: return "EQ B1 Gain";
            case EQ_B1_Q:    return "EQ B1 Q";
            case EQ_B1_Type: return "EQ B1 Type";
            case EQ_B2_Freq: return "EQ B2 Freq";
            case EQ_B2_Gain: return "EQ B2 Gain";
            case EQ_B2_Q:    return "EQ B2 Q";
            case EQ_B2_Type: return "EQ B2 Type";
            case PluginParam: return "Plugin Param";
            default:        return "Unknown";
        }
    }
};
