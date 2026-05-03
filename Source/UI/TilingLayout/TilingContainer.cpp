#include "TilingContainer.h"
#include "PanelHeader.h"

namespace TilingLayout
{
    TilingContainer::TilingContainer(juce::ValueTree nodeState, ContentProvider provider)
        : node(nodeState), contentProvider(provider)
    {
        node.state.addListener(this);
        triggerAsyncUpdate(); 
    }

    TilingContainer::~TilingContainer()
    {
        cancelPendingUpdate();
        node.state.removeListener(this);
    }

    void TilingContainer::handleAsyncUpdate()
    {
        updateStructure();
    }

    void TilingContainer::paint(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        const float gap = 4.0f;
        const float radius = 6.0f;
        auto panelArea = bounds.reduced(gap);

        if (node.getType() == Node::Leaf)
        {
            g.setColour(juce::Colour(45, 45, 45)); 
            g.fillRoundedRectangle(panelArea, radius);
            
            g.setColour(juce::Colour(60, 60, 60));
            g.drawRoundedRectangle(panelArea, radius, 1.0f);

            if (contentComponent == nullptr)
            {
                g.setColour(juce::Colours::white.withAlpha(0.1f));
                g.drawText("Empty Slot", panelArea, juce::Justification::centred);
            }
        }

        // Previsualización de Split (Blender Style)
        // Dibujar indicadores de esquina para que el usuario sepa dónde arrastrar
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        const float cSize = 15.0f;
        
        // Top-Left
        g.drawLine(gap, gap + cSize, gap + cSize, gap, 1.0f);
        // Top-Right
        g.drawLine(bounds.getWidth() - gap - cSize, gap, bounds.getWidth() - gap, gap + cSize, 1.0f);
        // Bottom-Left
        g.drawLine(gap, bounds.getHeight() - gap - cSize, gap + cSize, bounds.getHeight() - gap, 1.0f);
        // Bottom-Right
        g.drawLine(bounds.getWidth() - gap - cSize, bounds.getHeight() - gap, bounds.getWidth() - gap, bounds.getHeight() - gap - cSize, 1.0f);

        if (dragMode == CornerDragMode::Splitting)
        {
            g.setColour(juce::Colours::white.withAlpha(0.5f));
            auto p = getMouseXYRelative();
            if (isSplitVertical)
                g.drawVerticalLine((float)p.getX(), panelArea.getY(), panelArea.getBottom());
            else
                g.drawHorizontalLine((float)p.getY(), panelArea.getX(), panelArea.getRight());
        }
    }

    void TilingContainer::resized()
    {
        auto bounds = getLocalBounds();
        const int gap = 4;

        if (node.getType() == Node::Split)
        {
            if (child1 && child2 && splitter)
            {
                const int splitterSize = 2;
                float ratio = node.getRatio();

                if (node.getOrientation() == Node::Vertical)
                {
                    int w1 = (int)((float)bounds.getWidth() * ratio);
                    child1->setBounds(bounds.removeFromLeft(w1));
                    splitter->setBounds(bounds.removeFromLeft(splitterSize));
                    child2->setBounds(bounds);
                }
                else
                {
                    int h1 = (int)((float)bounds.getHeight() * ratio);
                    child1->setBounds(bounds.removeFromTop(h1));
                    splitter->setBounds(bounds.removeFromTop(splitterSize));
                    child2->setBounds(bounds);
                }
            }
        }
        else
        {
            auto panelArea = bounds.reduced(gap);
            if (header) header->setBounds(panelArea.removeFromTop(24));
            if (contentComponent)
            {
                contentComponent->setVisible(true);
                contentComponent->setBounds(panelArea);
            }
        }
    }

    void TilingContainer::updateStructure()
    {
        if (isUpdating) return;
        isUpdating = true;

        // CRÍTICO: Remover el componente físico de JUCE antes de perder su puntero
        if (contentComponent != nullptr) {
            removeChildComponent(contentComponent);
            contentComponent = nullptr;
        }
        ownedContent.reset(); // Liberar el puntero inteligente

        child1.reset();
        child2.reset();
        splitter.reset();
        header.reset();

        if (node.getType() == Node::Split)
        {
            child1 = std::make_unique<TilingContainer>(node.getChild(0), contentProvider);
            child2 = std::make_unique<TilingContainer>(node.getChild(1), contentProvider);
            splitter = std::make_unique<SplitterBar>(node.getOrientation());

            addAndMakeVisible(*child1);
            addAndMakeVisible(*child2);
            addAndMakeVisible(*splitter);

            splitter->onDrag = [this](int pos) { handleSplitterDrag(pos); };
        }
        else
        {
            auto id = node.getContentID();
            header = std::make_unique<PanelHeader>(id);
            addAndMakeVisible(*header);
            
            header->onTypeChanged = [this](juce::String newID) { 
                if (newID == "SplitH") node.split(Node::Horizontal, 0.5f, ContentID::Empty);
                else if (newID == "SplitV") node.split(Node::Vertical, 0.5f, ContentID::Empty);
                else node.setContentID(newID); 
            };
            
            header->onCloseRequested = [this]() {
                node.remove();
            };

            ownedContent = contentProvider(id);
            if (ownedContent)
            {
                contentComponent = ownedContent.get();
                addAndMakeVisible(contentComponent);
                contentComponent->toFront(false);
            }
        }
        
        isUpdating = false;
        resized();
    }

    bool TilingContainer::isMouseOverCorner(juce::Point<int> p)
    {
        if (node.getType() != Node::Leaf) return false;
        const int cornerSize = 60; // Área mucho más generosa para agarrar
        auto b = getLocalBounds();
        
        // Esquinas: Top-Left, Top-Right, Bottom-Left, Bottom-Right
        bool isNearLeft = p.getX() < cornerSize;
        bool isNearRight = p.getX() > b.getWidth() - cornerSize;
        bool isNearTop = p.getY() < cornerSize;
        bool isNearBottom = p.getY() > b.getHeight() - cornerSize;

        return (isNearLeft || isNearRight) && (isNearTop || isNearBottom);
    }

    void TilingContainer::mouseMove(const juce::MouseEvent& e)
    {
        bool overCorner = isMouseOverCorner(e.getPosition());
        auto targetCursor = overCorner ? juce::MouseCursor::CrosshairCursor : juce::MouseCursor::NormalCursor;
        if (getMouseCursor() != targetCursor) setMouseCursor(targetCursor);
    }

    void TilingContainer::mouseDown(const juce::MouseEvent& e)
    {
        if (isMouseOverCorner(e.getPosition()))
        {
            dragMode = CornerDragMode::Splitting;
            dragStartPos = e.getPosition();
            repaint();
        }
    }

    void TilingContainer::mouseDrag(const juce::MouseEvent& e)
    {
        if (dragMode == CornerDragMode::Splitting)
        {
            auto delta = e.getPosition() - dragStartPos;
            isSplitVertical = std::abs(delta.getX()) > std::abs(delta.getY());
            repaint();
        }
    }

    void TilingContainer::mouseUp(const juce::MouseEvent& e)
    {
        if (dragMode == CornerDragMode::Splitting)
        {
            dragMode = CornerDragMode::None;
            performCornerSplit(e.getPosition());
            repaint();
        }
    }

    void TilingContainer::performCornerSplit(juce::Point<int> endPos)
    {
        auto delta = endPos - dragStartPos;
        if (delta.getDistanceSquaredFromOrigin() < 100) return;

        float ratio = isSplitVertical 
            ? (float)endPos.getX() / (float)getWidth() 
            : (float)endPos.getY() / (float)getHeight();
        ratio = juce::jlimit(0.1f, 0.9f, ratio);
        
        node.split(isSplitVertical ? Node::Vertical : Node::Horizontal, ratio, ContentID::Empty);
    }

    void TilingContainer::handleSplitterDrag(int absolutePos)
    {
        float totalSize = (node.getOrientation() == Node::Vertical) ? (float)getWidth() : (float)getHeight();
        if (totalSize <= 0) return;
        float newRatio = juce::jlimit(0.05f, 0.95f, (float)absolutePos / totalSize);
        node.setRatio(newRatio);
    }

    void TilingContainer::valueTreePropertyChanged(juce::ValueTree& v, const juce::Identifier& i)
    {
        if (v == node.state) {
            if (i.toString() == "ratio") resized();
            else if (i.toString() == "type" || i.toString() == "contentID") triggerAsyncUpdate();
        }
    }

    void TilingContainer::valueTreeChildAdded(juce::ValueTree&, juce::ValueTree&) { triggerAsyncUpdate(); }
    void TilingContainer::valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree&, int) { triggerAsyncUpdate(); }
    void TilingContainer::valueTreeChildOrderChanged(juce::ValueTree&, int, int) { triggerAsyncUpdate(); }
}
