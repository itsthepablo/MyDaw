#pragma once
#include <JuceHeader.h>
#include "TilingLayoutNode.h"

namespace TilingLayout
{
    /**
     * Cabecera Minimalista estilo Blender.
     * Sin botones, solo el selector de tipo al hacer clic.
     */
    class PanelHeader : public juce::Component
    {
    public:
        PanelHeader(const juce::String& currentID) : panelID(currentID) {}

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            g.setColour(juce::Colour(40, 40, 40).withAlpha(0.8f));
            g.fillRect(b);

            // Icono de "Mover" (Grip handle) a la derecha, pero alejado de la esquina
            auto dragArea = getLocalBounds().withTrimmedRight(24).removeFromRight(24).toFloat();
            g.setColour(juce::Colours::white.withAlpha(isHoveringDrag ? 0.9f : 0.3f));
            float cx = dragArea.getCentreX();
            float cy = dragArea.getCentreY();
            g.drawLine(cx - 4, cy - 3, cx + 4, cy - 3, 2.0f);
            g.drawLine(cx - 4, cy,     cx + 4, cy,     2.0f);
            g.drawLine(cx - 4, cy + 3, cx + 4, cy + 3, 2.0f);

            // Texto del panel
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.setFont(juce::Font(11.0f));
            juce::String displayName = panelID;
            if (panelID == ContentID::Arrangement) displayName = "Playlist";
            
            g.drawText(displayName.toUpperCase(), getLocalBounds().withTrimmedLeft(28).withTrimmedRight(48).reduced(8, 0), juce::Justification::centredLeft);
        }

        void mouseMove(const juce::MouseEvent& e) override
        {
            bool overDrag = e.x >= getWidth() - 48 && e.x <= getWidth() - 24;
            if (isHoveringDrag != overDrag) {
                isHoveringDrag = overDrag;
                setMouseCursor(isHoveringDrag ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::PointingHandCursor);
                repaint();
            }
        }
        
        void mouseExit(const juce::MouseEvent& e) override
        {
            if (isHoveringDrag) {
                isHoveringDrag = false;
                repaint();
            }
        }

        void mouseDown(const juce::MouseEvent& e) override 
        { 
            if (e.x >= getWidth() - 48 && e.x <= getWidth() - 24) {
                isDraggingPanel = true;
                if (onPanelDragStart) onPanelDragStart();
            } else {
                showTypeMenu(); 
            }
        }
        
        void mouseDrag(const juce::MouseEvent& e) override
        {
            if (isDraggingPanel && onPanelDrag) {
                onPanelDrag(e.getScreenPosition());
            }
        }
        
        void mouseUp(const juce::MouseEvent& e) override
        {
            if (isDraggingPanel) {
                isDraggingPanel = false;
                if (onPanelDragEnd) onPanelDragEnd(e.getScreenPosition());
            }
        }

        std::function<void(juce::String)> onTypeChanged;
        std::function<void()> onCloseRequested;
        
        std::function<void()> onPanelDragStart;
        std::function<void(juce::Point<int>)> onPanelDrag;
        std::function<void(juce::Point<int>)> onPanelDragEnd;

    private:
        void showTypeMenu()
        {
            juce::PopupMenu m;
            m.addItem(1, "Playlist", true, panelID == ContentID::Arrangement);
            m.addItem(2, "Mixer", true, panelID == ContentID::Mixer);
            m.addItem(3, "Piano Roll", true, panelID == ContentID::PianoRoll);
            m.addItem(4, "Left Sidebar", true, panelID == ContentID::LeftSidebar);
            m.addItem(5, "Right Dock", true, panelID == ContentID::RightDock);
            m.addItem(6, "Bottom Dock", true, panelID == ContentID::BottomDock);
            
            m.addSeparator();
            m.addItem(99, "Close View", true);

            m.showMenuAsync(juce::PopupMenu::Options(), [this](int result) {
                if (result == 0) return;
                
                if (result == 99) {
                    if (onCloseRequested) onCloseRequested();
                    return;
                }

                juce::String ids[] = { "", ContentID::Arrangement, ContentID::Mixer, ContentID::PianoRoll, ContentID::LeftSidebar, ContentID::RightDock, ContentID::BottomDock };
                if (onTypeChanged) onTypeChanged(ids[result]);
            });
        }

        juce::String panelID;
        bool isHoveringDrag = false;
        bool isDraggingPanel = false;
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelHeader)
    };
}
