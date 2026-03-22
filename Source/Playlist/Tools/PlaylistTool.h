#pragma once
#include <JuceHeader.h>

class PlaylistComponent; // Declaración anticipada para evitar dependencias circulares

class PlaylistTool {
public:
    virtual ~PlaylistTool() = default;
    
    // Todas las herramientas deben implementar estos 4 métodos
    virtual void mouseDown(const juce::MouseEvent& e, PlaylistComponent& p) = 0;
    virtual void mouseDrag(const juce::MouseEvent& e, PlaylistComponent& p) = 0;
    virtual void mouseUp(const juce::MouseEvent& e, PlaylistComponent& p) = 0;
    virtual void mouseMove(const juce::MouseEvent& e, PlaylistComponent& p) = 0;
};