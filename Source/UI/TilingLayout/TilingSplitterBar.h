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
            if (isHovered || isDragging)
            {
                // Barra gruesa y visible cuando el ratón está encima o arrastrando
                g.setColour(juce::Colour(80, 150, 255).withAlpha(0.8f)); // Azul acento
                g.fillAll();
            }
            else
            {
                // Dejamos el resto transparente (hitbox invisible) y solo dibujamos una línea fina en el centro
                g.setColour(juce::Colour(15, 15, 15));
                auto bounds = getLocalBounds();
                
                if (orientation == Node::Vertical)
                    g.fillRect(bounds.getWidth() / 2 - 1, 0, 2, bounds.getHeight());
                else
                    g.fillRect(0, bounds.getHeight() / 2 - 1, bounds.getWidth(), 2);
            }
        }

        void mouseEnter(const juce::MouseEvent&) override
        {
            isHovered = true;
            repaint();
        }

        void mouseExit(const juce::MouseEvent&) override
        {
            isHovered = false;
            repaint();
        }

        void mouseDown(const juce::MouseEvent& e) override
        {
            isDragging = true;
            repaint();
            if (onDragStart) onDragStart();
        }

        void mouseUp(const juce::MouseEvent& e) override
        {
            isDragging = false;
            repaint();
        }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            if (onDrag)
            {
                auto pos = e.getEventRelativeTo(getParentComponent()).getPosition();
                onDrag(orientation == Node::Vertical ? pos.getX() : pos.getY());
            }
        }

        std::function<void()> onDragStart;
        std::function<void(int)> onDrag;

    private:
        Node::Orientation orientation;
        bool isHovered = false;
        bool isDragging = false;
    };
}
