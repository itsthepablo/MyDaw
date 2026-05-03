#pragma once
#include <JuceHeader.h>
#include "TilingLayoutNode.h"
#include "TilingSplitterBar.h"
#include "PanelHeader.h"
#include "ComponentFactory.h"

namespace TilingLayout
{
    class TilingContainer : public juce::Component, 
                            public juce::ValueTree::Listener,
                            public juce::AsyncUpdater
    {
    public:
        using ContentProvider = std::function<TilingContent(const juce::String&)>;

        TilingContainer(juce::ValueTree nodeState, ContentProvider provider);
        ~TilingContainer();

        void paint(juce::Graphics& g) override;
        void resized() override;
        
        void handleAsyncUpdate() override;

        // Eventos de ratón para esquinas
        void mouseMove(const juce::MouseEvent& e) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;

        // ValueTree::Listener
        void valueTreePropertyChanged(juce::ValueTree& v, const juce::Identifier& i) override;
        void valueTreeChildAdded(juce::ValueTree& p, juce::ValueTree& c) override;
        void valueTreeChildRemoved(juce::ValueTree& p, juce::ValueTree& c, int i) override;
        void valueTreeChildOrderChanged(juce::ValueTree& p, int oldIndex, int newIndex) override;

    private:
        void updateStructure();
        void handleSplitterDrag(int delta);
        
        // --- LÓGICA DE ESQUINAS (BLENDER STYLE) ---
        bool isMouseOverCorner(juce::Point<int> p);
        void handleCornerDrag(const juce::MouseEvent& e);
        void performCornerSplit(juce::Point<int> endPos);

        Node node;
        ContentProvider contentProvider;

        enum class CornerDragMode { None, PotentialSplit, Splitting };
        CornerDragMode dragMode = CornerDragMode::None;
        juce::Point<int> dragStartPos;
        bool isSplitVertical = false;

        std::unique_ptr<TilingContainer> child1;
        std::unique_ptr<TilingContainer> child2;
        std::unique_ptr<SplitterBar> splitter;
        std::unique_ptr<PanelHeader> header;
        
        juce::Component* contentComponent = nullptr;
        TilingContent ownedContent;
        bool isUpdating = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TilingContainer)
    };
}
