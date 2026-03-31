#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "../Mixer/MixerComponent.h"
#include "../Playlist/PlaylistComponent.h"

class TrackMixerPlaylistBridge {
public:
    static void connect(TrackContainer& container,
        MixerComponent& mixer,
        MixerComponent& miniMixer,
        PlaylistComponent& playlist,
        Track* masterTrack)
    {
        // Pasamos la referencia de los tracks a ambos mixers
        mixer.setMasterTrack(masterTrack);
        miniMixer.setMasterTrack(masterTrack);
        
        mixer.setTracksReference(&(container.getTracks()));
        miniMixer.setTracksReference(&(container.getTracks()));
        playlist.setTracksReference(&(container.getTracks()));

        // Sincronización del Scroll Vertical
        playlist.onVerticalScroll = [&container](int scrollY) {
            container.setVOffset(scrollY);
            };

        // --- SOLUCIÓN AL BUG DEL SCROLL y OVERWRITE DEL AUDIO ENGINE ---
        auto prevOnTrack = container.onTrackAdded;
        container.onTrackAdded = [&playlist, prevOnTrack] {
            if (prevOnTrack) prevOnTrack();
            playlist.updateScrollBars();
            playlist.repaint();
            };

        // Cuando se reordenan (Drag & Drop)
        auto prevOnReorder = container.onTracksReordered;
        container.onTracksReordered = [&container, &mixer, &miniMixer, &playlist, prevOnReorder] {
            if (prevOnReorder) prevOnReorder();
            mixer.setTracksReference(&(container.getTracks()));
            miniMixer.setTracksReference(&(container.getTracks()));
            playlist.updateScrollBars();
            playlist.repaint();
            };
    }
};