#pragma once
#include <JuceHeader.h>

// 1. INCLUDES DE UI PRIMERO (CRÍTICO PARA QUE AUDIOENGINE COMPILE)
#include "UI/UIManager.h" 
#include "UI/Layout/LayoutHandler.h"
#include "RenderEngine/OfflineRenderer.h"
#include "RenderEngine/StemRenderer/StemRendererWindow.h"
#include "RenderEngine/RenderController.h"

// 2. INCLUDES RESTANTES DESPUÉS DE LA UI
#include "UI/Layout/ViewManager.h"
#include "Project/ProjectController.h"
#include "Engine/Core/AudioEngine.h"
#include "Bridge/BridgeManager.h"
#include "UI/Commands/CommandController.h"
#include "Theme/ThemeManagerWindow.h" // NUEVO: Ventana de temas
#include "Tracks/TrackManager.h"
#include "Tracks/SelectionManager.h"

class MainComponent : public juce::AudioAppComponent,
    public juce::ApplicationCommandTarget,
    public juce::DragAndDropContainer
{
public:
    MainComponent();
    ~MainComponent() override;

    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

    void toggleViewMode() { viewManager.toggleViewMode(); }

    void loadProject(const juce::File& file) { projectController.loadProject(file, [this] { resized(); }); }

    // --- DELEGACIÓN DE COMANDOS ---
    juce::ApplicationCommandTarget* getNextCommandTarget() override { return commandController.get(); }
    void getAllCommands(juce::Array<juce::CommandID>& c) override { commandController->getAllCommands(c); }
    void getCommandInfo(juce::CommandID id, juce::ApplicationCommandInfo& r) override { commandController->getCommandInfo(id, r); }
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& i) override { return commandController->perform(i); }


private:
    void setupCallbacks();
    void setupBridges();
    void setupCommands();


    std::unique_ptr<CommandController> commandController;

    DAWUIComponents ui;
    ViewManager viewManager{ ui, *this };
    TrackManager trackManager{ ui, audioEngine, audioMutex };
    SelectionManager selectionManager{ ui, audioEngine };
public:
    ProjectController projectController{ ui, audioEngine, audioMutex };
private:

    juce::CriticalSection audioMutex;
    AudioEngine audioEngine;

    // --- RENDER ---
    RenderController renderController{ ui, audioEngine, *this, audioMutex, isOfflineRendering };
    std::atomic<bool> isOfflineRendering{ false };

    std::unique_ptr<ThemeManagerWindow> themeWindow; // NUEVO: Instancia persistente

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};