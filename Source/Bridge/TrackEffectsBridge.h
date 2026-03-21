#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Effects/EffectsPanel.h"
#include "../PluginHost/VSTHost.h"

class TrackEffectsBridge {
public:
    static void connect(TrackContainer& container,
        EffectsPanel& panel,
        juce::CriticalSection& audioMutex,
        double& sampleRate,
        int& blockSize,
        bool& isVisible,
        std::function<void()> triggerResize)
    {
        // 1. Cerrar el panel desde la 'X' de la UI lateral
        panel.onClose = [&isVisible, triggerResize] {
            isVisible = false;
            triggerResize();
            };

        // 2. Añadir efecto desde el panel lateral (clic en slot vacío)
        panel.onAddEffect = [&panel, &container, &audioMutex, &sampleRate, &blockSize](Track& t) {
            auto* host = new VSTHost();
            host->loadPluginAsync(sampleRate, [&t, host, &panel, &container, &audioMutex, sampleRate, blockSize](bool success) {
                if (success) {
                    host->prepareToPlay(sampleRate, blockSize);
                    const juce::ScopedLock sl(audioMutex);
                    t.plugins.add(host);
                    EffectsPanel::pluginIsInstrumentMap[(void*)host] = false;
                    panel.updateSlots();
                    container.refreshTrackPanels();
                }
                else { delete host; }
                });
            };

        // 3. Abrir ventana del plugin desde el panel lateral
        panel.onOpenEffect = [](Track& t, int idx) {
            if (idx < t.plugins.size()) t.plugins[idx]->showWindow();
            };

        // 4. Reordenar efectos (Drag and Drop en el panel)
        panel.onReorderEffects = [&audioMutex, &panel, &container](Track& t, int oldIdx, int newIdx) {
            const juce::ScopedLock sl(audioMutex);
            t.plugins.move(oldIdx, newIdx);
            panel.updateSlots();
            container.refreshTrackPanels();
            };

        // 5. CONEXIÓN CLAVE: Abrir el panel desde el botón FX de un track
        container.onOpenEffectsPanel = [&panel, &isVisible, triggerResize](Track& t) {
            panel.setTrack(&t);     // Enfocamos el panel en esta pista
            isVisible = true;       // Cambiamos el estado a visible
            triggerResize();        // Forzamos el redibujado para que la Playlist se encoja
            };

        // 6. Cargar Instrumento o FX directamente desde el track
        container.onLoadFx = [&sampleRate, &blockSize, &audioMutex, &panel](Track& t, TrackControlPanel& trackPanel) {
            auto* host = new VSTHost();
            host->loadPluginAsync(sampleRate, [&t, &trackPanel, host, &panel, &audioMutex, sampleRate, blockSize](bool success) {
                if (success) {
                    host->prepareToPlay(sampleRate, blockSize);
                    const juce::ScopedLock sl(audioMutex);
                    int insertIdx = 0;
                    for (int i = 0; i < t.plugins.size(); ++i) {
                        if (EffectsPanel::pluginIsInstrumentMap[(void*)t.plugins[i]]) insertIdx = i + 1;
                        else break;
                    }
                    t.plugins.insert(insertIdx, host);
                    EffectsPanel::pluginIsInstrumentMap[(void*)host] = true;
                    trackPanel.updatePlugins();
                    if (panel.getActiveTrack() == &t) panel.updateSlots();
                }
                else { delete host; }
                });
            };

        // 7. Abrir ventana del plugin desde el track
        container.onOpenFx = [](Track& t, int idx) {
            if (idx < (int)t.plugins.size()) t.plugins[idx]->showWindow();
            };
    }
};