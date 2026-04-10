#pragma once
#include <JuceHeader.h>

// 1. INCLUDES DE UI PRIMERO (CRÍTICO PARA QUE AUDIOENGINE COMPILE)
#include "UI/UIManager.h" 
#include "UI/Layout/LayoutHandler.h"
#include "RenderEngine/OfflineRenderer.h" // --- INYECCIÓN 1: Ventana de render ---
#include "RenderEngine/StemRenderer/StemRendererWindow.h" // NUEVO STEM RENDERER

// 2. INCLUDES RESTANTES DESPUÉS DE LA UI
#include "Project/ProjectManager.h"
#include "Engine/Core/AudioEngine.h"
#include "Bridge/BridgeManager.h"
#include "UI/Commands/DAWCommandHandler.h"
#include "Theme/ThemeManagerWindow.h" // NUEVO: Ventana de temas

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

    juce::ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& c) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const juce::ApplicationCommandTarget::InvocationInfo& info) override;

    void toggleViewMode();
    void loadProject(const juce::File& file);

private:
    void setupCallbacks();
    void setupBridges();
    void setupCommands();

    void openPianoRoll();
    void closePianoRoll();
    void saveProject();
    void startExport(); // --- INYECCIÓN 2: Función ejecutora ---
    
    // --- NUEVO: Exportación Múltiple (Stems) ---
    void showStemRenderer();

    juce::ApplicationCommandManager commandManager;
    std::unique_ptr<DAWCommandHandler> commandHandler;

    ViewMode currentView = ViewMode::Arrangement;
    DAWUIComponents ui;

    juce::CriticalSection audioMutex;
    AudioEngine audioEngine;

    // --- INYECCIÓN 3: Variables del Render ---
    std::unique_ptr<OfflineRenderer> offlineRenderer;
    std::atomic<bool> isOfflineRendering{ false };
    
    // --- NUEVO: ESTADO DEL RENDERING MULTITRACK (STEMS) ---
    std::unique_ptr<StemRendererHost> stemRendererWindow;
    std::map<int, bool> preRenderMuteStates;
    std::map<int, bool> preRenderSoloStates;
    // ----------------------------------------

    bool isBottomDockVisible = true;
    int bottomDockHeight = 300;
    bool isLeftSidebarVisible = true;
    int leftSidebarWidth = 200;

    bool isPianoRollVisible = false;
    bool prePianoRollLeftSidebar = true;
    bool prePianoRollBottomDock = true;

    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<ThemeManagerWindow> themeWindow; // NUEVO: Instancia persistente

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};