#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include "MixerChannelUI.h" 

// ==============================================================================
// VIEWPORT SIN INTERCEPCION DE DRAG
// El Viewport de JUCE por defecto intercepta los eventos mouseDrag para
// hacer scroll, lo que impide que los Sliders hijos reciban esos eventos.
// Esta subclase deshabilita ese comportamiento para que el drag llegue
// directamente a los sliders (faders y knobs).
// ==============================================================================
class NonDraggableViewport : public juce::Viewport {
public:
    using juce::Viewport::Viewport;

    void mouseDrag(const juce::MouseEvent& e) override {
        // No propagar al Viewport base: eso causaria scroll y robaria
        // el evento de los Sliders hijos. Los hijos ya reciben el evento
        // directamente via el mecanismo de propagacion de JUCE.
        juce::ignoreUnused(e);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        // Tampoco interceptar mouseDown para scroll.
        juce::ignoreUnused(e);
    }
};

// ==============================================================================
// CONTENEDOR PRINCIPAL DEL MIXER 
// ==============================================================================
class MixerComponent : public juce::Component, private juce::Timer {
public:
    MixerComponent();
    ~MixerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateChannels();

    bool isMiniMixer = false;

    void timerCallback() override;

    void setTracksReference(const juce::OwnedArray<Track>* tracks) {
        tracksRef = tracks;
        updateChannels();
    }

    void setMasterTrack(Track* master) {
        masterTrackPtr = master;
        updateChannels();
    }

    float getMasterVolume() const { return masterVolume; }
    void setMasterVolume(float v) { masterVolume = v; }
    void setAudioMutex(juce::CriticalSection* m) { audioMutex = m; }

    // --- CALLBACKS PARA EL SISTEMA ---
    std::function<void(Track&)> onAddVST3;
    std::function<void(Track&)> onAddNativeUtility;
    std::function<void(Track&, int)> onOpenPlugin;
    std::function<void(Track&, int)> onDeleteEffect;
    std::function<void(Track&, int, bool)> onBypassChanged;
    
    std::function<void(Track&)> onAddSend;
    std::function<void(Track&, int)> onDeleteSend;

    std::function<void(Track&, int, juce::String)> onCreateAutomation;

private:
    const juce::OwnedArray<Track>* tracksRef = nullptr;
    juce::CriticalSection* audioMutex = nullptr;
    Track* masterTrackPtr = nullptr;
    float masterVolume = 1.0f;

    NonDraggableViewport viewport;
    juce::Component contentComp;
    juce::OwnedArray<MixerChannelUI> channels;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};