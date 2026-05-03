#pragma once
#include <JuceHeader.h>
#include "TilingLayoutNode.h"

namespace TilingLayout
{
    class SplitterBar : public juce::Component
    {
    public:
        SplitterBar(Node::Orientation o) : orientation(o)
        {
            setMouseCursor(orientation == Node::Vertical 
                ? juce::MouseCursor::LeftRightResizeCursor 
                : juce::MouseCursor::UpDownResizeCursor);
        }

        void paint(juce::Graphics& g) override
        {
            // Color de fondo oscuro sólido para el separador
            g.setColour(juce::Colour(15, 15, 15));
            g.fillAll();
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            if (onDragStart) onDragStart();
        }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            if (onDrag)
            {
                // Pasamos la posición absoluta del ratón respecto al padre
                auto pos = e.getEventRelativeTo(getParentComponent()).getPosition();
                onDrag(orientation == Node::Vertical ? pos.getX() : pos.getY());
            }
        }

        std::function<void()> onDragStart;
        std::function<void(int)> onDrag;

    private:
        Node::Orientation orientation;
    };
}
