#pragma once
#include <JuceHeader.h>

class TransportBar : public juce::Component {
public:
    TransportBar() {
        // El selector de BPM se movió a TopMenuBar (Estilo FL Studio Drag)
    }

    void resized() override {
        auto area = getLocalBounds();

        // Diseño de la barra (Los botones migraron a TopMenuBar)
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TransportBar)
};