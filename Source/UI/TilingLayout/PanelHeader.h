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
        PanelHeader(const juce::String& currentID) : panelID(currentID)
        {
            setMouseCursor(juce::MouseCursor::PointingHandCursor);
        }

        void paint(juce::Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
            
            // Fondo oscuro integrado
            g.setColour(juce::Colour(40, 40, 40).withAlpha(0.8f));
            g.fillRect(b);

            // Icono / Texto del panel
            g.setColour(juce::Colours::white.withAlpha(0.6f));
            g.setFont(juce::Font(11.0f));
            
            juce::String displayName = panelID;
            if (panelID == ContentID::Arrangement) displayName = "Playlist";
            
            g.drawText(displayName.toUpperCase(), getLocalBounds().reduced(8, 0), juce::Justification::centredLeft);
        }

        void mouseDown(const juce::MouseEvent& e) override { showTypeMenu(); }

        std::function<void(juce::String)> onTypeChanged;
        std::function<void()> onCloseRequested;

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
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanelHeader)
    };
}
