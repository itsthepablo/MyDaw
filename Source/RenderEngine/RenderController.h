#pragma once
#include <JuceHeader.h>
#include <map>
#include <atomic>
#include <vector>

// Forward declarations
class DAWUIComponents;
class AudioEngine;
class OfflineRenderer;
class StemRendererHost;

/**
 * RenderController: Encapsula la logica de exportacion de audio (mix completo y stems).
 * Desacopla el motor de renderizado de los componentes de la interfaz principal.
 */
class RenderController
{
public:
    RenderController(DAWUIComponents& ui,
                     AudioEngine& audioEngine,
                     juce::AudioAppComponent& appComponent,
                     juce::CriticalSection& audioMutex,
                     std::atomic<bool>& isOfflineRendering);
                     
    ~RenderController();

    void startExport(juce::Component* parentComponent);
    void showStemRenderer(juce::Component* parentComponent);

private:
    DAWUIComponents&         ui;
    AudioEngine&             audioEngine;
    juce::AudioAppComponent& appComponent;
    juce::CriticalSection&   audioMutex;
    std::atomic<bool>&       isOfflineRendering;

    std::unique_ptr<OfflineRenderer>    offlineRenderer;
    std::unique_ptr<StemRendererHost>   stemRendererWindow;
    std::map<int, bool>                 preRenderMuteStates;
    std::map<int, bool>                 preRenderSoloStates;

    void ensureOfflineRenderer(juce::Component* parent);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RenderController)
};
