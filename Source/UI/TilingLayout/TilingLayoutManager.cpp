#include "TilingLayoutManager.h"
#include "../Layout/ArrangementPanel.h"
#include "../../Engine/Core/AudioEngine.h"

namespace TilingLayout
{
    TilingLayoutManager::TilingLayoutManager(DAWUIComponents& uiComponents, AudioEngine& engine, juce::CriticalSection& mutex)
        : ui(uiComponents), audioEngine(engine), audioMutex(mutex), factory(uiComponents, engine, mutex)
    {
        startTimerHz(15);
    }


    void TilingLayoutManager::setArrangementPanel(ArrangementPanel* ap)
    {
        arrangementPanel = ap;
        factory.setMasterArrangement(ap);
        loadDefaultLayout();
    }

    void TilingLayoutManager::loadDefaultLayout()
    {
        rootState = Node::createLeaf(ContentID::Arrangement);
        createRootContainer();
    }

    void TilingLayoutManager::createRootContainer()
    {
        rootContainer = std::make_unique<TilingContainer>(rootState, [this](const juce::String& id) {
            return factory.createComponent(id);
        });

        addAndMakeVisible(*rootContainer);
        resized();
    }

    void TilingLayoutManager::resized()
    {
        if (rootContainer)
            rootContainer->setBounds(getLocalBounds());
    }

    void TilingLayoutManager::splitActivePanel(Node::Orientation o)
    {
    }
}
