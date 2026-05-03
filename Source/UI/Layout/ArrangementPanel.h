#pragma once
#include <JuceHeader.h>
#include "../../Tracks/UI/TrackContainer.h"
#include "../../Playlist/PlaylistComponent.h"
#include "../../Engine/Core/AudioEngine.h"

namespace TilingLayout
{
    /**
     * ArrangementPanel - REPARACIÓN INTEGRAL.
     * Restaura el Drag & Drop y la Sincronización de Audio.
     */
    class ArrangementPanel : public juce::Component, private juce::Timer
    {
    public:
        ArrangementPanel(TrackContainer& tc, PlaylistComponent& pc, AudioEngine& engine, juce::CriticalSection& mutex)
            : trackContainer(&tc), playlist(&pc), audioEngine(engine)
        {
            setup();
            startTimerHz(30); // Timer para sincronización de estado global
        }

        ArrangementPanel(juce::OwnedArray<Track>* tracks, AudioEngine& engine, juce::CriticalSection& mutex)
            : audioEngine(engine)
        {
            ownedTC = std::make_unique<TrackContainer>();
            ownedPC = std::make_unique<PlaylistComponent>();
            
            trackContainer = ownedTC.get();
            playlist = ownedPC.get();

            trackContainer->setExternalTracks(tracks);
            trackContainer->setExternalMutex(&mutex);
            playlist->setTracksReference(tracks);
            playlist->setExternalMutex(&mutex);
            
            setup();
            trackContainer->refreshTrackPanels();
            startTimerHz(30);
        }

        ~ArrangementPanel() override { stopTimer(); }

        void setup()
        {
            addAndMakeVisible(*trackContainer);
            addAndMakeVisible(*playlist);

            // CRÍTICO: Re-establecer la conexión para el Drag & Drop
            playlist->setTrackContainer(trackContainer);

            // Sincronización de posición
            playlist->getPlaybackPosition = [this] { 
                return audioEngine.transportState.currentAudioPlayhead.load(); 
            };
            
            playlist->onVerticalScroll = [this](int scrollPos) {
                trackContainer->setVOffset(scrollPos);
            };

            // El Seek en los espejos ahora es SEGURO porque no compite por la duración
            playlist->onPlayheadSeekRequested = [this](float pos) { 
                audioEngine.transportState.seekRequestPh.store(pos); 
            };

            trackContainer->onScrollWheel = [this](float deltaY) {
                double currentStart = playlist->vBar.getCurrentRangeStart();
                double currentSize = playlist->vBar.getCurrentRangeSize();
                double maxLimit = playlist->vBar.getMaximumRangeLimit();
                double newStart = juce::jlimit(0.0, std::max(0.0, maxLimit - currentSize), currentStart - (deltaY * 100.0));
                playlist->vBar.setCurrentRange(newStart, currentSize);
                playlist->updateScrollBars();
                playlist->repaint();
            };
        }

        void timerCallback() override
        {
            const bool enginePlaying = audioEngine.transportState.isPlaying.load();
            if (playlist->isPlaying != enginePlaying) {
                playlist->isPlaying = enginePlaying;
            }
            
            // Refresco de scroll y dibujo para que los clones sean instantáos
            playlist->updateScrollBars();
            playlist->repaint();
            trackContainer->repaint();
        }

        void resized() override
        {
            auto area = getLocalBounds();
            trackContainer->setBounds(area.removeFromLeft(250));
            playlist->setBounds(area);
        }

        TrackContainer* getTrackContainer() { return trackContainer; }
        PlaylistComponent* getPlaylist() { return playlist; }

    private:
        AudioEngine& audioEngine;
        TrackContainer* trackContainer = nullptr;
        PlaylistComponent* playlist = nullptr;

        std::unique_ptr<TrackContainer> ownedTC;
        std::unique_ptr<PlaylistComponent> ownedPC;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrangementPanel)
    };
}
