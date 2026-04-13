#pragma once
#include <JuceHeader.h>

/**
 * SmartColorUtils: Utilidad para asignar colores basados en el nombre.
 */
class SmartColorUtils {
public:
    static juce::Colour getColorForName(const juce::String& name)
    {
        juce::String n = name.toLowerCase().trim();

        // Drums - Rojo
        if (n.contains("kick") || n.contains("bombo") || n.contains("drum") || n.contains("perc"))
            return juce::Colour(220, 60, 60);

        // Snare/Clap - Amarillo/Naranja
        if (n.contains("snare") || n.contains("caja") || n.contains("clap") || n.contains("rim"))
            return juce::Colour(240, 180, 40);

        // HiHats/Cymbals - Cian
        if (n.contains("hat") || n.contains("hihat") || n.contains("cymbal") || n.contains("ride") || n.contains("crash"))
            return juce::Colour(60, 200, 220);

        // Bass - Verde
        if (n.contains("bass") || n.contains("bajo") || n.contains("808") || n.contains("sub"))
            return juce::Colour(60, 180, 100);

        // Lead/Synth - Violeta
        if (n.contains("lead") || n.contains("synth") || n.contains("pad") || n.contains("poly") || n.contains("keys"))
            return juce::Colour(160, 80, 240);

        // Vox - Rosa
        if (n.contains("vox") || n.contains("vocal") || n.contains("voice") || n.contains("coro"))
            return juce::Colour(240, 100, 180);

        // FX - Gris/Plateado
        if (n.contains("fx") || n.contains("effect") || n.contains("noise") || n.contains("riser"))
            return juce::Colour(150, 150, 160);

        return juce::Colour(); // Retorna color nulo (no detectado)
    }
};
