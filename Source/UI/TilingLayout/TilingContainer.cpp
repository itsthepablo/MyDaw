#include "TilingContainer.h"
#include "PanelHeader.h"

namespace TilingLayout
{
    TilingContainer::TilingContainer(juce::ValueTree nodeState, ContentProvider provider)
        : node(nodeState), contentProvider(provider)
    {
        for (int i = 0; i < 4; ++i) {
            corners[i] = std::make_unique<CornerGrabber>(*this, i);
            addAndMakeVisible(corners[i].get());
        }
        
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
        const float gap = 1.0f;
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
    }

    void TilingContainer::paintOverChildren(juce::Graphics& g)
    {
        if (dragMode == CornerDragMode::Splitting)
        {
            auto bounds = getLocalBounds().toFloat();
            const float gap = 4.0f;
            const float radius = 6.0f;
            auto panelArea = bounds.reduced(gap);
            
            auto p = getMouseXYRelative();
            
            // La línea de división
            g.setColour(juce::Colours::white.withAlpha(0.8f));
            if (isSplitVertical)
                g.drawVerticalLine((float)p.getX(), panelArea.getY(), panelArea.getBottom());
            else
                g.drawHorizontalLine((float)p.getY(), panelArea.getX(), panelArea.getRight());
                
            // Rectángulo azul de previsualización para la NUEVA ventana (Child 2)
            g.setColour(juce::Colour(80, 150, 255).withAlpha(0.25f)); 
            
            juce::Rectangle<float> newWindowArea;
            if (isSplitVertical)
                newWindowArea = juce::Rectangle<float>((float)p.getX(), panelArea.getY(), panelArea.getRight() - (float)p.getX(), panelArea.getHeight());
            else
                newWindowArea = juce::Rectangle<float>(panelArea.getX(), (float)p.getY(), panelArea.getWidth(), panelArea.getBottom() - (float)p.getY());
            
            newWindowArea = newWindowArea.getIntersection(panelArea);
            g.fillRoundedRectangle(newWindowArea, radius);
        }
    }

    void TilingContainer::resized()
    {
        auto bounds = getLocalBounds();
        const int gap = 1;

        if (node.getType() == Node::Split)
        {
            for (int i = 0; i < 4; ++i) corners[i]->setVisible(false);
            
            if (child1 && child2 && splitter)
            {
                const int layoutSplitterSize = 1;
                float ratio = node.getRatio();

                if (node.getOrientation() == Node::Vertical)
                {
                    int w1 = (int)((float)bounds.getWidth() * ratio);
                    child1->setBounds(bounds.removeFromLeft(w1));
                    
                    auto layoutSplitArea = bounds.removeFromLeft(layoutSplitterSize);
                    auto hitBoxArea = layoutSplitArea;
                    hitBoxArea.expand(3, 0); // Hitbox invisible de 7px para agarrar fácil
                    splitter->setBounds(hitBoxArea);
                    
                    child2->setBounds(bounds);
                }
                else
                {
                    int h1 = (int)((float)bounds.getHeight() * ratio);
                    child1->setBounds(bounds.removeFromTop(h1));
                    
                    auto layoutSplitArea = bounds.removeFromTop(layoutSplitterSize);
                    auto hitBoxArea = layoutSplitArea;
                    hitBoxArea.expand(0, 3);
                    splitter->setBounds(hitBoxArea);
                    
                    child2->setBounds(bounds);
                }
                splitter->toFront(false);
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

            // Posicionar los corners
            const int cSize = 24; // Área de 24x24 px reales y prioritarios
            corners[0]->setBounds(0, 0, cSize, cSize); // Top-Left
            corners[1]->setBounds(bounds.getWidth() - cSize, 0, cSize, cSize); // Top-Right
            corners[2]->setBounds(0, bounds.getHeight() - cSize, cSize, cSize); // Bottom-Left
            corners[3]->setBounds(bounds.getWidth() - cSize, bounds.getHeight() - cSize, cSize, cSize); // Bottom-Right
            
            for (int i = 0; i < 4; ++i) {
                corners[i]->setVisible(true);
                corners[i]->toFront(false);
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

    void TilingContainer::CornerGrabber::mouseDown(const juce::MouseEvent& e) { parent.cornerMouseDown(e.getEventRelativeTo(&parent)); }
    void TilingContainer::CornerGrabber::mouseDrag(const juce::MouseEvent& e) { parent.cornerMouseDrag(e.getEventRelativeTo(&parent)); }
    void TilingContainer::CornerGrabber::mouseUp(const juce::MouseEvent& e) { parent.cornerMouseUp(e.getEventRelativeTo(&parent)); }
    
    void TilingContainer::CornerGrabber::paint(juce::Graphics& g)
    {
        // Icono visual: Pequeño triángulo sutil estilo Blender en la punta de la esquina
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        juce::Path p;
        if (cornerType == 0) { // Top-Left
            p.addTriangle(2, 2, 12, 2, 2, 12);
        } else if (cornerType == 1) { // Top-Right
            p.addTriangle(22, 2, 12, 2, 22, 12);
        } else if (cornerType == 2) { // Bottom-Left
            p.addTriangle(2, 22, 12, 22, 2, 12);
        } else if (cornerType == 3) { // Bottom-Right
            p.addTriangle(22, 22, 12, 22, 22, 12);
        }
        
        // Efecto hover para que sea más visible si el ratón está encima
        if (isMouseOver()) {
            g.setColour(juce::Colours::white.withAlpha(0.6f));
        }
        
        g.fillPath(p);
    }

    void TilingContainer::cornerMouseDown(const juce::MouseEvent& e)
    {
        if (node.getType() != Node::Leaf) return;
        dragMode = CornerDragMode::Splitting;
        dragStartPos = e.getPosition();
    }

    void TilingContainer::cornerMouseDrag(const juce::MouseEvent& e)
    {
        if (dragMode == CornerDragMode::Splitting)
        {
            auto delta = e.getPosition() - dragStartPos;
            isSplitVertical = std::abs(delta.getX()) > std::abs(delta.getY());
            repaint();
        }
    }

    void TilingContainer::cornerMouseUp(const juce::MouseEvent& e)
    {
        if (dragMode == CornerDragMode::Splitting)
        {
            performCornerSplit(e.getPosition());
            dragMode = CornerDragMode::None;
            repaint();
        }
    }

    void TilingContainer::mouseMove(const juce::MouseEvent& e)
    {
        // Ya no es necesario manejar isMouseOverCorner aquí, los CornerGrabbers gestionan el cursor
    }

    void TilingContainer::mouseDown(const juce::MouseEvent& e)
    {
    }

    void TilingContainer::mouseDrag(const juce::MouseEvent& e)
    {
    }

    void TilingContainer::mouseUp(const juce::MouseEvent& e)
    {
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
        
        float oldRatio = node.getRatio();
        float oldW1 = totalSize * oldRatio;
        float idealNewW1 = absolutePos;
        float delta = idealNewW1 - oldW1; 
        
        if (delta == 0.0f) return;

        // Función para descubrir cuánto puede encogerse un sub-árbol antes de golpear su límite
        std::function<float(juce::ValueTree, Node::Orientation, float, bool)> getAvailableShrink = 
            [&getAvailableShrink](juce::ValueTree v, Node::Orientation parentOri, float currentSize, bool shrinkFromLeft) -> float
        {
            Node n(v);
            float minSize = std::max(30.0f, currentSize * 0.05f); // 5% o 30 píxeles mínimo
            
            if (n.getType() == Node::Leaf || n.getOrientation() != parentOri) {
                return std::max(0.0f, currentSize - minSize);
            }

            float r = n.getRatio();
            if (shrinkFromLeft) {
                // El impacto viene por la izquierda, lo absorbe el hijo izquierdo
                return getAvailableShrink(n.getChild(0), parentOri, currentSize * r, true);
            } else {
                // El impacto viene por la derecha, lo absorbe el hijo derecho
                return getAvailableShrink(n.getChild(1), parentOri, currentSize * (1.0f - r), false);
            }
        };

        // Pre-calcular el límite máximo de arrastre para no aplastar ventanas
        if (delta > 0) 
        {
            float maxShrink = getAvailableShrink(node.getChild(1), node.getOrientation(), totalSize - oldW1, true);
            if (delta > maxShrink) delta = maxShrink;
        }
        else 
        {
            float maxShrink = getAvailableShrink(node.getChild(0), node.getOrientation(), oldW1, false);
            if (-delta > maxShrink) delta = -maxShrink;
        }

        // Aplicar el delta seguro
        float newW1 = oldW1 + delta;
        float newRatio = juce::jlimit(0.05f, 0.95f, newW1 / totalSize);
        node.setRatio(newRatio);

        // Recalcular el delta real aplicado (por si jlimit lo redujo aún más)
        delta = (totalSize * newRatio) - oldW1;

        // Lambda recursiva para compensar los paneles anidados
        std::function<void(juce::ValueTree, Node::Orientation, float, float, bool)> compensate = 
            [&compensate](juce::ValueTree v, Node::Orientation parentOri, float sizeChange, float oldTotalSize, bool absorbOnLeft) 
        {
            if (sizeChange == 0.0f) return;
            Node n(v);
            if (n.getType() != Node::Split) return;
            if (n.getOrientation() != parentOri) return; 

            float newTotalSize = oldTotalSize + sizeChange;
            if (newTotalSize <= 1.0f) return;

            float r = n.getRatio();
            float oldChild0Size = oldTotalSize * r;

            if (absorbOnLeft) 
            {
                float newChild0Size = oldChild0Size + sizeChange;
                float newR = juce::jlimit(0.05f, 0.95f, newChild0Size / newTotalSize);
                n.setRatio(newR);
                compensate(n.getChild(0), parentOri, sizeChange, oldChild0Size, true);
            }
            else 
            {
                float newChild0Size = oldChild0Size;
                float newR = juce::jlimit(0.05f, 0.95f, newChild0Size / newTotalSize);
                n.setRatio(newR);
                float oldChild1Size = oldTotalSize - oldChild0Size;
                compensate(n.getChild(1), parentOri, sizeChange, oldChild1Size, false);
            }
        };

        // El hijo izquierdo (child0) cambia su tamaño en +delta. 
        // Su lado izquierdo (pegado al borde lejano) no debe moverse, así que absorbe por la derecha (absorbOnLeft = false).
        compensate(node.getChild(0), node.getOrientation(), delta, oldW1, false);

        // El hijo derecho (child1) cambia su tamaño en -delta. 
        // Su lado derecho (pegado al borde lejano) no debe moverse, así que absorbe por la izquierda (absorbOnLeft = true).
        compensate(node.getChild(1), node.getOrientation(), -delta, totalSize - oldW1, true);
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
