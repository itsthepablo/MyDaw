#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Mixer/MixerComponent.h"
#include "../Playlist/PlaylistComponent.h"

class TrackMixerPlaylistBridge {
public:
    static void connect(TrackContainer& container,
        MixerComponent& mixer,
        PlaylistComponent& playlist)
    {
        // Pasamos la referencia de los tracks
        mixer.setTracksReference(&(container.getTracks()));
        playlist.setTracksReference(&(container.getTracks()));

        // Sincronización del Scroll Vertical
        playlist.onVerticalScroll = [&container](int scrollY) {
            container.setVOffset(scrollY);
            };

        // --- SOLUCIÓN AL BUG DEL SCROLL ---
        // Cuando se añade un track, forzamos a la playlist a recalcular el scroll
        container.onTrackAdded = [&playlist] {
            playlist.updateScrollBars();
            playlist.repaint();
            };

        // Cuando se reordenan (Drag & Drop)
        container.onTracksReordered = [&container, &mixer, &playlist] {
            mixer.setTracksReference(&(container.getTracks()));
            playlist.updateScrollBars();
            playlist.repaint();
            };
    }
};