#pragma once
#include <JuceHeader.h>
#include "../BaseEffect.h"

class NodePatcher;
class PatcherEngine;

// ==============================================================================
// DATA STRUCTURES
// ==============================================================================
struct PatcherNode {
    juce::Uuid id;
    juce::String name;
    juce::Point<int> position { 100, 100 };
    std::shared_ptr<BaseEffect> effect; 
    bool isInput = false;
    bool isOutput = false;
};

struct PatcherConnection {
    juce::Uuid id;
    juce::Uuid sourceId;
    juce::Uuid destId;
    float volume = 1.0f;
};

// ==============================================================================
// EDITOR UI
// ==============================================================================
class NodePatcherEditor : public juce::Component {
public:
    NodePatcherEditor(NodePatcher* patcher);
    ~NodePatcherEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    void updateNodes();

private:
    NodePatcher* owner;
    
    juce::Uuid draggingSourceNode;
    juce::Point<int> dragCurrentPos;
    bool isDraggingCable = false;

    class CableKnob : public juce::Slider {
    public:
        juce::Uuid connId;
        NodePatcherEditor* editor;
        CableKnob(NodePatcherEditor* e, juce::Uuid id);
        void mouseDown(const juce::MouseEvent& e) override;
    };

    juce::OwnedArray<CableKnob> cableKnobs;
    void updateCablePositions();

    class NodeUI : public juce::Component {
    public:
        NodeUI(PatcherNode* nodeData, NodePatcherEditor* parentEditor);
        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
        
        PatcherNode* data;
        NodePatcherEditor* editor;
        juce::ComponentDragger dragger;
        juce::Component* customEditor = nullptr;
        
        void mouseDoubleClick(const juce::MouseEvent& e) override;
        void resized() override;
    };

    juce::OwnedArray<NodeUI> nodeUIs;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NodePatcherEditor)
};

// ==============================================================================
// WINDOW
// ==============================================================================
class NodePatcherEditorWindow : public juce::DocumentWindow {
public:
    NodePatcherEditorWindow(NodePatcher* pluginOwner);
    ~NodePatcherEditorWindow() override;

    void closeButtonPressed() override;
    void refreshUI() { editor.updateNodes(); }

private:
    NodePatcher* owner;
    NodePatcherEditor editor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NodePatcherEditorWindow)
};

// ==============================================================================
// NATIVE PLUGIN CLASS
// ==============================================================================
class NodePatcher : public BaseEffect {
public:
    NodePatcher();
    ~NodePatcher() override;

    bool isLoaded() const override { return true; }
    juce::String getLoadedPluginName() const override { return "Node Patcher"; }

    bool isBypassed() const override { return bypassed; }
    void setBypassed(bool shouldBypass) override { bypassed = shouldBypass; }

    void showWindow(TrackContainer* container = nullptr) override;

    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void reset() override;

    int getLatencySamples() const override { return 0; }
    
    bool isNative() const override { return true; }
    juce::Component* getCustomEditor() override { return nullptr; } 

    void windowClosed();

    // --- Graph Data ---
    std::vector<PatcherNode> nodes;
    std::vector<PatcherConnection> connections;

    void addNode(juce::String name, std::shared_ptr<BaseEffect> effect, juce::Point<int> pos);
    void connectNodes(juce::Uuid src, juce::Uuid dst);
    void removeConnection(juce::Uuid connId);
    PatcherNode* getNode(juce::Uuid id);
    PatcherConnection* getConnection(juce::Uuid id);

    void triggerUIRefresh();

    // Callbacks para cargar plugins reales desde el puente
    std::function<void(juce::Point<int>)> onAddNativeUtilityRequest;
    std::function<void(juce::Point<int>)> onAddVST3Request;

private:
    bool bypassed = false;
    std::unique_ptr<NodePatcherEditorWindow> editorWindow;
    std::unique_ptr<PatcherEngine> engine;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NodePatcher)
};

