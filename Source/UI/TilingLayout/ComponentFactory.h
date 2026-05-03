#pragma once
#include <JuceHeader.h>
#include "../UIManager.h"
#include "../../Mixer/MixerComponent.h"
#include "../../Playlist/PlaylistComponent.h"
#include "../../Tracks/UI/TrackContainer.h"
#include "../Layout/ArrangementPanel.h"
#include "../../PianoRoll/PianoRollComponent.h"

// Forward declaration para evitar dependencias circulares
class AudioEngine;

namespace TilingLayout
{
    /**
     * TilingContent es un alias para un puntero inteligente que sabe 
     * si debe borrar el componente o no al destruirse.
     */
    using TilingContent = std::unique_ptr<juce::Component, std::function<void(juce::Component*)>>;

    class ComponentFactory
    {
    public:
        ComponentFactory(DAWUIComponents& uiRef, AudioEngine& engine, juce::CriticalSection& mutex) 
            : ui(uiRef), audioEngine(engine), audioMutex(mutex) {}

        void setMasterArrangement(ArrangementPanel* master) { masterArrangement = master; }

        TilingContent createComponent(const juce::String& id)
        {
            auto noopDeleter = [](juce::Component*) {};
            auto defaultDeleter = [](juce::Component* c) { delete c; };

            // --- COMPONENTES DUPLICABLES ---
            
            if (id == ContentID::Mixer)
            {
                auto m = new MixerComponent();
                m->setTracksReference(&ui.trackContainer.getTracks());
                return TilingContent(m, defaultDeleter);
            }

            if (id == ContentID::PianoRoll)
            {
                auto pr = new PianoRollComponent();
                pr->setBpm(ui.pianoRollUI.getBpm());
                pr->setActiveClip(ui.pianoRollUI.getActiveClip());
                return TilingContent(pr, defaultDeleter);
            }

            if (id == ContentID::Arrangement)
            {
                // Si la maestra está libre (no tiene padre), la usamos para evitar "fantasmas"
                if (masterArrangement != nullptr && masterArrangement->getParentComponent() == nullptr) 
                    return TilingContent(masterArrangement, noopDeleter);

                // Si la maestra ya está ocupada, creamos un CLON sincronizado
                auto ap = new ArrangementPanel(&ui.trackContainer.getTracksMutable(), audioEngine, audioMutex);
                
                // Sincronización crucial: Compartir datos y comportamiento
                ap->getTrackContainer()->copyCallbacksFrom(ui.trackContainer);
                ap->getPlaylist()->copyCallbacksFrom(ui.playlistUI);
                
                return TilingContent(ap, defaultDeleter);
            }

            // --- COMPONENTES ÚNICOS ---
            
            if (id == ContentID::LeftSidebar) return TilingContent(&ui.leftSidebar, noopDeleter);
            if (id == ContentID::Playlist) return TilingContent(&ui.playlistUI, noopDeleter);
            if (id == ContentID::TrackContainer) return TilingContent(&ui.trackContainer, noopDeleter);
            if (id == ContentID::RightDock) return TilingContent(&ui.rightDock, noopDeleter);
            if (id == ContentID::BottomDock) return TilingContent(&ui.bottomDock, noopDeleter);
            
            return TilingContent(nullptr, noopDeleter);
        }

    private:
        DAWUIComponents& ui;
        AudioEngine& audioEngine;
        juce::CriticalSection& audioMutex;
        ArrangementPanel* masterArrangement = nullptr;
    };
}
