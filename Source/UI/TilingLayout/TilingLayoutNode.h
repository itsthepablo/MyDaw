#pragma once
#include <JuceHeader.h>

namespace TilingLayout
{
    /**
     * Identificadores para los componentes que pueden estar en un panel.
     */
    namespace ContentID
    {
        const juce::String PianoRoll = "PianoRoll";
        const juce::String Mixer = "Mixer";
        const juce::String Playlist = "Playlist";
        const juce::String Arrangement = "Arrangement";
        const juce::String TrackContainer = "TrackContainer";
        const juce::String LeftSidebar = "LeftSidebar";
        const juce::String RightDock = "RightDock";
        const juce::String BottomDock = "BottomDock";
        const juce::String MasterChannel = "MasterChannel";
        const juce::String Empty = "Empty";
    }

    /**
     * Wrapper de ValueTree para un nodo del layout.
     * Un nodo puede ser un SPLIT (contiene dos hijos) o un LEAF (contiene un panel).
     */
    struct Node
    {
        Node(const juce::ValueTree& v) : state(v) {}

        enum Type { Split, Leaf };
        enum Orientation { Vertical, Horizontal };

        static juce::ValueTree createLeaf(const juce::String& contentID)
        {
            juce::ValueTree v("Node");
            v.setProperty("type", (int)Leaf, nullptr);
            v.setProperty("contentID", contentID, nullptr);
            return v;
        }

        static juce::ValueTree createSplit(Orientation o, float ratio, juce::ValueTree child1, juce::ValueTree child2)
        {
            juce::ValueTree v("Node");
            v.setProperty("type", (int)Split, nullptr);
            v.setProperty("orientation", (int)o, nullptr);
            v.setProperty("ratio", ratio, nullptr);
            v.addChild(child1, -1, nullptr);
            v.addChild(child2, -1, nullptr);
            return v;
        }

        bool isValid() const { return state.isValid(); }
        Type getType() const { return (Type)(int)state.getProperty("type"); }
        Orientation getOrientation() const { return (Orientation)(int)state.getProperty("orientation"); }
        float getRatio() const { return (float)state.getProperty("ratio", 0.5f); }
        void setRatio(float r) { state.setProperty("ratio", r, nullptr); }
        
        juce::String getContentID() const { return state.getProperty("contentID").toString(); }
        void setContentID(const juce::String& id) { state.setProperty("contentID", id, nullptr); }

        juce::ValueTree getChild(int index) const { return state.getChild(index); }
        
        void split(Orientation o, float ratio, const juce::String& newContentID, bool newIsFirst = false)
        {
            if (getType() != Leaf) return;

            juce::ValueTree oldLeaf = state.createCopy();
            state.setProperty("type", (int)Split, nullptr);
            state.setProperty("orientation", (int)o, nullptr);
            state.setProperty("ratio", ratio, nullptr);
            state.removeProperty("contentID", nullptr);
            
            if (newIsFirst) {
                state.addChild(createLeaf(newContentID), -1, nullptr);
                state.addChild(oldLeaf, -1, nullptr);
            } else {
                state.addChild(oldLeaf, -1, nullptr);
                state.addChild(createLeaf(newContentID), -1, nullptr);
            }
        }

        void remove()
        {
            auto parentTree = state.getParent();
            if (!parentTree.isValid()) return; // No se puede borrar el nodo raíz (a menos que lo permitamos, pero mejor no)

            // Buscar al hermano y clonarlo
            juce::ValueTree sibling = parentTree.getChild(0) == state ? parentTree.getChild(1).createCopy() : parentTree.getChild(0).createCopy();
            
            // Reemplazar al padre con el hermano
            parentTree.removeAllProperties(nullptr);
            parentTree.removeAllChildren(nullptr);
            
            parentTree.copyPropertiesFrom(sibling, nullptr);
            for (int i = 0; i < sibling.getNumChildren(); ++i) {
                parentTree.addChild(sibling.getChild(i).createCopy(), -1, nullptr);
            }
        }

        juce::ValueTree state;
    };
}
