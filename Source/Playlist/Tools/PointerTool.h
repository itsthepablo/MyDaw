#pragma once
#include <JuceHeader.h>
#include "PlaylistTool.h"
#include "../../Clips/Midi/UI/MidiPatternRenderer.h"
#include "../../Clips/Audio/AudioClip.h"
#include "../../Clips/Midi/MidiPattern.h"
#include "../../Clips/Audio/UI/AudioClipActions.h"

// Forward declaration para evitar dependencia circular
class PlaylistComponent;

/**
 * PointerTool — Herramienta de selección y edición estándar del Playlist.
 * Maneja clics en clips, notas inline, automatización y menús contextuales.
 */
class PointerTool : public PlaylistTool {
public:
    PointerTool() = default;
    ~PointerTool() override = default;

    void mouseDown(const juce::MouseEvent& e, PlaylistComponent& p) override;
    void mouseDrag(const juce::MouseEvent& e, PlaylistComponent& p) override;
    void mouseUp(const juce::MouseEvent& e, PlaylistComponent& p) override;
    void mouseMove(const juce::MouseEvent& e, PlaylistComponent& p) override;
};