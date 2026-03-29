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

        // --- SOLUCIÓN AL BUG DEL SCROLL y OVERWRITE DEL AUDIO ENGINE ---
        // Concatenamos las funciones para no borrar las conexiones hechas en MainComponent
        auto prevOnTrack = container.onTrackAdded;
        container.onTrackAdded = [&playlist, prevOnTrack] {
            if (prevOnTrack) prevOnTrack();
            playlist.updateScrollBars();
            playlist.repaint();
            };

        // Cuando se reordenan (Drag & Drop)
        auto prevOnReorder = container.onTracksReordered;
        container.onTracksReordered = [&container, &mixer, &playlist, prevOnReorder] {
            if (prevOnReorder) prevOnReorder();
            mixer.setTracksReference(&(container.getTracks()));
            playlist.updateScrollBars();
            playlist.repaint();
            };
    }
};