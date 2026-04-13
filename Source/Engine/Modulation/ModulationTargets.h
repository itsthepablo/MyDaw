#pragma once
#include <JuceHeader.h>

/**
 * ModTarget: Estructura unificada para identificar cualquier parámetro modulable en el DAW.
 */
struct ModTarget {
    enum Type { 
        None = 0, 
        Volume, 
        Pan, 
        PluginParam, 
        PreGain, 
        PostGain 
    };

    Type type = None;
    int pluginIdx = -1;    // Índice del plugin en la cadena de efectos de la pista
    int parameterIdx = -1; // Índice del parámetro dentro del plugin (VstParameterIndex)
    
    // Para simplificar comparaciones rápidas e identificadores únicos
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
        return type == Volume || type == Pan || type == PreGain || type == PostGain;
    }

    bool isValid() const {
        return type != None;
    }
};
