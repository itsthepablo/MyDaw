#pragma once
#include <JuceHeader.h>
#include "TilingContainer.h"
#include "ComponentFactory.h"
#include "../UIManager.h"

// Forward declaration
class AudioEngine;

namespace TilingLayout
{
    class ArrangementPanel;

    class TilingLayoutManager : public juce::Component, private juce::Timer
    {
    public:
        TilingLayoutManager(DAWUIComponents& uiRef, AudioEngine& engine, juce::CriticalSection& mutex);
        ~TilingLayoutManager() override { stopTimer(); }

        void timerCallback() override { repaint(); }

        void resized() override;
        
        void setArrangementPanel(ArrangementPanel* ap);
        void loadDefaultLayout();
        void createRootContainer();

    private:
        void splitActivePanel(Node::Orientation o);

        DAWUIComponents& ui;
        AudioEngine& audioEngine;
        juce::CriticalSection& audioMutex;
        
        ArrangementPanel* arrangementPanel = nullptr;
        ComponentFactory factory;
        juce::ValueTree rootState;
        std::unique_ptr<TilingContainer> rootContainer;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TilingLayoutManager)
    };
}
