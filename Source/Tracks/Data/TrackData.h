#pragma once

/**
 * @enum TrackType
 * Representa los diferentes tipos de pistas dentro del DAW.
 */
enum class TrackType { 
    MIDI, 
    Audio, 
    Folder, 
    Loudness, 
    Balance, 
    MidSide 
};

/**
 * @enum WaveformViewMode
 * Modos de visualización de la forma de onda en la playlist.
 */
enum class WaveformViewMode { 
    Combined, 
    SeparateLR, 
    MidSide, 
    Spectrogram 
};
