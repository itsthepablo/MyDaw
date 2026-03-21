#pragma once
#include <JuceHeader.h>

/**
 * Puente de Sincronización: Conecta los datos de los Tracks con el Mixer y la Playlist.
 */
class TrackMixerPlaylistBridge {
public:
    static void connect(TrackContainer& container, 
                        MixerComponent& mixer, 
                        PlaylistComponent& playlist) 
    {
        // 1. Sincronización inicial: pasamos la dirección de memoria de la lista de tracks
        mixer.setTracksReference(&(container.getTracks()));
        playlist.setTracksReference(&(container.getTracks()));

        // 2. Cuando el usuario reordena tracks (Drag & Drop), avisamos a ambos
        container.onTracksReordered = [&container, &mixer, &playlist] {
            mixer.setTracksReference(&(container.getTracks()));
            playlist.repaint();
        };
    }
};