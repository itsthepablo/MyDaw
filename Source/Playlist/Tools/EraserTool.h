#pragma once
#include <JuceHeader.h>
#include "PlaylistTool.h"
#include "../PlaylistComponent.h"

class EraserTool : public PlaylistTool {
public:
    void mouseDown(const juce::MouseEvent& e, PlaylistComponent& p) override {
        // Obtenemos el clip debajo del ratón
        int cIdx = p.getClipAt(e.x, e.y);
        
        // Si tocamos un clip válido, lo borramos inmediatamente
        if (cIdx != -1) {
            p.deleteClip(cIdx);
        }
    }
    
    void mouseDrag(const juce::MouseEvent& e, PlaylistComponent& p) override {
        // Podríamos implementar en el futuro que si arrastras el borrador, borre varios
    }
    
    void mouseUp(const juce::MouseEvent& e, PlaylistComponent& p) override {}
    
    void mouseMove(const juce::MouseEvent& e, PlaylistComponent& p) override {
        int cIdx = p.getClipAt(e.x, e.y);
        if (cIdx != -1) {
            // Si pasamos el borrador sobre un clip, mostramos una cruz para indicar peligro
            p.setMouseCursor(juce::MouseCursor::CrosshairCursor);
        } else {
            p.setMouseCursor(juce::MouseCursor::NormalCursor);
        }
    }
};