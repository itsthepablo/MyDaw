#include "NodePatcher.h"
#include "PatcherEngine.h"

// ==============================================================================
// NodeUI Implementation
// ==============================================================================
NodePatcherEditor::NodeUI::NodeUI(PatcherNode* nodeData, NodePatcherEditor* parentEditor)
    : data(nodeData), editor(parentEditor)
{
    setSize(120, 50);
    
    if (data->effect && data->effect->isNative()) {
        customEditor = data->effect->getCustomEditor();
        if (customEditor) {
            if (customEditor->getWidth() == 0) customEditor->setSize(120, 60);
            addAndMakeVisible(customEditor);
            setSize(juce::jmax(160, customEditor->getWidth() + 40), customEditor->getHeight() + 30);
        }
    }
}

void NodePatcherEditor::NodeUI::resized()
{
    if (customEditor) {
        customEditor->setBounds(20, 20, getWidth() - 40, getHeight() - 30);
    }
}

void NodePatcherEditor::NodeUI::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Sombra
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRoundedRectangle(bounds.translated(2, 2), 6.0f);
    
    // Fondo
    juce::Colour bgCol = data->isInput || data->isOutput ? juce::Colour(40, 45, 55) : juce::Colour(50, 55, 65);
    g.setColour(bgCol);
    g.fillRoundedRectangle(bounds, 6.0f);
    
    // Borde
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);
    
    // Texto
    g.setColour(juce::Colours::white);
    g.setFont(juce::Font(14.0f, juce::Font::bold));
    
    if (customEditor) {
        // Cabecera superior
        g.drawText(data->name, 0, 0, getWidth(), 20, juce::Justification::centred);
    } else {
        g.drawText(data->name, bounds, juce::Justification::centred);
        if (data->effect && !data->effect->isNative()) {
            g.setFont(10.0f);
            g.setColour(juce::Colours::grey);
            g.drawText("(Doble click para abrir)", bounds.translated(0, 15), juce::Justification::centred);
        }
    }

    // Pines de conexión (Output a la derecha)
    if (!data->isOutput) {
        g.setColour(juce::Colours::cyan);
        g.fillEllipse(getWidth() - 10, getHeight() / 2 - 5, 10, 10);
    }
    // Pines de conexión (Input a la izquierda)
    if (!data->isInput) {
        g.setColour(juce::Colours::cyan);
        g.fillEllipse(0, getHeight() / 2 - 5, 10, 10);
    }
}

void NodePatcherEditor::NodeUI::mouseDown(const juce::MouseEvent& e)
{
    // Si hace click en el pin derecho, inicia cable
    if (!data->isOutput && e.getMouseDownX() > getWidth() - 15) {
        editor->mouseDown(e.getEventRelativeTo(editor));
        return;
    }
    dragger.startDraggingComponent(this, e);
}

void NodePatcherEditor::NodeUI::mouseDoubleClick(const juce::MouseEvent& e)
{
    if (data->effect && !data->effect->isNative()) {
        data->effect->showWindow();
    }
}

void NodePatcherEditor::NodeUI::mouseDrag(const juce::MouseEvent& e)
{
    if (!data->isOutput && e.getMouseDownX() > getWidth() - 15) {
        editor->mouseDrag(e.getEventRelativeTo(editor));
        return;
    }
    dragger.dragComponent(this, e, nullptr);
    data->position = getPosition();
    editor->updateCablePositions();
    editor->repaint(); // Redibujar cables
}

void NodePatcherEditor::NodeUI::mouseUp(const juce::MouseEvent& e)
{
    editor->mouseUp(e.getEventRelativeTo(editor));
}

// ==============================================================================
// CableKnob Implementation
// ==============================================================================
NodePatcherEditor::CableKnob::CableKnob(NodePatcherEditor* e, juce::Uuid id)
    : juce::Slider(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox), connId(id), editor(e) {}

void NodePatcherEditor::CableKnob::mouseDown(const juce::MouseEvent& e) {
    if (e.mods.isRightButtonDown()) {
        juce::PopupMenu m;
        m.addItem(1, "Desconectar Cable");
        m.showMenuAsync(juce::PopupMenu::Options(), [this](int r) {
            if (r == 1) {
                editor->owner->removeConnection(connId);
            }
        });
        return;
    }
    juce::Slider::mouseDown(e);
}

// ==============================================================================
// NodePatcherEditor Implementation
// ==============================================================================
NodePatcherEditor::NodePatcherEditor(NodePatcher* patcher) : owner(patcher)
{
    updateNodes();
}

NodePatcherEditor::~NodePatcherEditor() {}

void NodePatcherEditor::updateNodes()
{
    nodeUIs.clear();
    cableKnobs.clear();

    for (auto& node : owner->nodes) {
        auto* ui = new NodeUI(&node, this);
        ui->setTopLeftPosition(node.position);
        nodeUIs.add(ui);
        addAndMakeVisible(ui);
    }

    for (auto& conn : owner->connections) {
        auto* slider = new CableKnob(this, conn.id);
        slider->setRange(0.0, 2.0);
        slider->setValue(conn.volume, juce::dontSendNotification);
        slider->setTooltip("Cable Volume");
        slider->onValueChange = [this, id = conn.id, slider] {
            if (auto* c = owner->getConnection(id)) {
                c->volume = (float)slider->getValue();
            }
        };
        cableKnobs.add(slider);
        addAndMakeVisible(slider);
    }

    updateCablePositions();
    repaint();
}

void NodePatcherEditor::updateCablePositions() {
    for (int i = 0; i < owner->connections.size(); ++i) {
        auto& conn = owner->connections[i];
        if (i < cableKnobs.size()) {
            NodeUI* srcUI = nullptr;
            NodeUI* dstUI = nullptr;
            for (auto* ui : nodeUIs) {
                if (ui->data->id == conn.sourceId) srcUI = ui;
                if (ui->data->id == conn.destId) dstUI = ui;
            }
            if (srcUI && dstUI) {
                juce::Point<float> start(srcUI->getRight() - 5, srcUI->getBounds().getCentreY());
                juce::Point<float> end(dstUI->getX() + 5, dstUI->getBounds().getCentreY());
                
                juce::Point<float> mid;
                if (end.x < start.x + 40.0f) {
                    float outX = start.x + 30.0f;
                    float inX = end.x - 30.0f;
                    float midY = (start.y + end.y) / 2.0f;
                    if (std::abs(start.y - end.y) < 40) midY = start.y + 60.0f;
                    mid = juce::Point<float>((outX + inX) / 2.0f, midY);
                } else {
                    mid = juce::Point<float>((start.x + end.x) / 2.0f, (start.y + end.y) / 2.0f);
                }
                
                cableKnobs[i]->setBounds((int)mid.x - 15, (int)mid.y - 15, 30, 30);
            }
        }
    }
}

void NodePatcherEditor::paint(juce::Graphics& g)
{
    // Grid de fondo
    g.fillAll(juce::Colour(25, 28, 33));
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    for (int i = 0; i < getWidth(); i += 20) g.drawVerticalLine(i, 0, getHeight());
    for (int i = 0; i < getHeight(); i += 20) g.drawHorizontalLine(i, 0, getWidth());

    // Helper lambda para dibujar cables ortogonales
    auto drawOrthogonalCable = [&](juce::Point<float> start, juce::Point<float> end, float alpha) {
        juce::Path cable;
        cable.startNewSubPath(start);

        float midX = (start.x + end.x) / 2.0f;
        if (end.x < start.x + 40.0f) {
            float outX = start.x + 30.0f;
            float inX = end.x - 30.0f;
            float midY = (start.y + end.y) / 2.0f;
            if (std::abs(start.y - end.y) < 40) midY = start.y + 60.0f;

            cable.lineTo(outX, start.y);
            cable.lineTo(outX, midY);
            cable.lineTo(inX, midY);
            cable.lineTo(inX, end.y);
            cable.lineTo(end);
        } else {
            cable.lineTo(midX, start.y);
            cable.lineTo(midX, end.y);
            cable.lineTo(end);
        }

        cable = cable.createPathWithRoundedCorners(12.0f);
        g.setColour(juce::Colours::cyan.withAlpha(alpha));
        g.strokePath(cable, juce::PathStrokeType(2.5f));
    };

    // Dibujar conexiones existentes
    for (auto& conn : owner->connections) {
        NodeUI* srcUI = nullptr;
        NodeUI* dstUI = nullptr;
        for (auto* ui : nodeUIs) {
            if (ui->data->id == conn.sourceId) srcUI = ui;
            if (ui->data->id == conn.destId) dstUI = ui;
        }

        if (srcUI && dstUI) {
            juce::Point<float> start(srcUI->getRight() - 5, srcUI->getBounds().getCentreY());
            juce::Point<float> end(dstUI->getX() + 5, dstUI->getBounds().getCentreY());
            drawOrthogonalCable(start, end, 0.8f);
        }
    }

    // Dibujar cable en progreso
    if (isDraggingCable) {
        NodeUI* srcUI = nullptr;
        for (auto* ui : nodeUIs) {
            if (ui->data->id == draggingSourceNode) srcUI = ui;
        }
        if (srcUI) {
            juce::Point<float> start(srcUI->getRight() - 5, srcUI->getBounds().getCentreY());
            juce::Point<float> end((float)dragCurrentPos.x, (float)dragCurrentPos.y);
            drawOrthogonalCable(start, end, 0.5f);
        }
    }
}

void NodePatcherEditor::resized() {}

void NodePatcherEditor::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown()) {
        juce::PopupMenu m;
        m.addItem(1, "Native: Utility (Gain/Pan)");
        m.addSeparator();
        m.addItem(2, "External VST3...");
        
        m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(
            { e.getScreenPosition().x, e.getScreenPosition().y, 1, 1 }), 
            [this, pos = e.getPosition()](int r) {
                if (r == 1 && owner->onAddNativeUtilityRequest) owner->onAddNativeUtilityRequest(pos);
                if (r == 2 && owner->onAddVST3Request) owner->onAddVST3Request(pos);
            });
        return;
    }

    // Identificar si iniciamos un drag de cable desde el pin de un nodo
    for (auto* ui : nodeUIs) {
        if (!ui->data->isOutput) {
            juce::Rectangle<int> pinBounds(ui->getRight() - 15, ui->getY(), 15, ui->getHeight());
            if (pinBounds.contains(e.getPosition())) {
                draggingSourceNode = ui->data->id;
                isDraggingCable = true;
                dragCurrentPos = e.getPosition();
                return;
            }
        }
    }
}

void NodePatcherEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingCable) {
        dragCurrentPos = e.getPosition();
        repaint();
    }
}

void NodePatcherEditor::mouseUp(const juce::MouseEvent& e)
{
    if (isDraggingCable) {
        isDraggingCable = false;
        
        // Comprobar si soltamos el cable sobre el pin de entrada de otro nodo
        for (auto* ui : nodeUIs) {
            if (!ui->data->isInput && ui->data->id != draggingSourceNode) {
                juce::Rectangle<int> pinBounds(ui->getX(), ui->getY(), 15, ui->getHeight());
                if (pinBounds.contains(e.getPosition())) {
                    owner->connectNodes(draggingSourceNode, ui->data->id);
                    break;
                }
            }
        }
        repaint();
    }
}

// ==============================================================================
// NodePatcherEditorWindow Implementation
// ==============================================================================
NodePatcherEditorWindow::NodePatcherEditorWindow(NodePatcher* pluginOwner)
    : juce::DocumentWindow("Node Patcher - Parallel FX Rack",
                           juce::Colour(30, 33, 36),
                           juce::DocumentWindow::allButtons),
      owner(pluginOwner), editor(pluginOwner)
{
    setUsingNativeTitleBar(false);
    setResizable(true, true);
    setResizeLimits(600, 400, 1920, 1080);
    setContentNonOwned(&editor, false);
    centreWithSize(800, 600);
}

NodePatcherEditorWindow::~NodePatcherEditorWindow()
{
}

void NodePatcherEditorWindow::closeButtonPressed()
{
    if (owner) owner->windowClosed();
}

// ==============================================================================
// NodePatcher Implementation
// ==============================================================================
NodePatcher::NodePatcher()
{
    // Crear nodos por defecto (Audio In y Audio Out)
    PatcherNode inNode;
    inNode.id = juce::Uuid();
    inNode.name = "Audio In";
    inNode.position = { 50, 250 };
    inNode.isInput = true;
    nodes.push_back(inNode);

    PatcherNode outNode;
    outNode.id = juce::Uuid();
    outNode.name = "Audio Out";
    outNode.position = { 600, 250 };
    outNode.isOutput = true;
    nodes.push_back(outNode);
    
    engine = std::make_unique<PatcherEngine>(this);
    engine->compileGraph();
}

NodePatcher::~NodePatcher()
{
    editorWindow.reset();
}

void NodePatcher::showWindow(TrackContainer* container)
{
    if (editorWindow == nullptr) {
        editorWindow = std::make_unique<NodePatcherEditorWindow>(this);
    }
    
    editorWindow->setVisible(true);
    editorWindow->toFront(true);
}

void NodePatcher::windowClosed()
{
    if (editorWindow != nullptr) {
        editorWindow->setVisible(false);
    }
}

void NodePatcher::triggerUIRefresh()
{
    if (editorWindow != nullptr) {
        juce::MessageManager::callAsync([this]() {
            if (editorWindow) editorWindow->refreshUI();
        });
    }
}

void NodePatcher::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock)
{
    engine->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
}

void NodePatcher::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (bypassed) return;
    engine->processBlock(buffer, midiMessages);
}

void NodePatcher::reset()
{
    // Limpiar buffers o estado si es necesario
}

void NodePatcher::addNode(juce::String name, std::shared_ptr<BaseEffect> effect, juce::Point<int> pos)
{
    PatcherNode n;
    n.id = juce::Uuid();
    n.name = name;
    n.effect = effect;
    n.position = pos;
    nodes.push_back(n);
    engine->compileGraph();
    triggerUIRefresh();
}

void NodePatcher::connectNodes(juce::Uuid src, juce::Uuid dst)
{
    // Evitar duplicados
    for (auto& c : connections) {
        if (c.sourceId == src && c.destId == dst) return;
    }
    connections.push_back({juce::Uuid(), src, dst, 1.0f});
    engine->compileGraph();
    triggerUIRefresh();
}

void NodePatcher::removeConnection(juce::Uuid connId) {
    connections.erase(std::remove_if(connections.begin(), connections.end(), 
        [&](const PatcherConnection& c) { return c.id == connId; }), connections.end());
    engine->compileGraph();
    triggerUIRefresh();
}

PatcherNode* NodePatcher::getNode(juce::Uuid id)
{
    for (auto& n : nodes) if (n.id == id) return &n;
    return nullptr;
}

PatcherConnection* NodePatcher::getConnection(juce::Uuid id) {
    for (auto& c : connections) if (c.id == id) return &c;
    return nullptr;
}
