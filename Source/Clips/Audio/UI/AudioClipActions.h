#pragma once
#include <JuceHeader.h>
#include "../AudioClip.h"
#include "../AudioClipStyles.h"
#include "../../../Playlist/PlaylistComponent.h"

/**
 * AudioClipActions: Clase estática que centraliza las acciones y menús del AudioClip.
 * Evita la saturación de los archivos de herramientas (como PointerTool.h).
 */
class AudioClipActions {
public:
    static void showContextMenu(AudioClip* linkedAudio, PlaylistComponent& p, const juce::MouseEvent& e, int cIdx) {
        if (linkedAudio == nullptr) return;

        juce::PopupMenu m;
        
        // --- SUBMENU: ESTILO DE FORMA DE ONDA ---
        juce::PopupMenu styleMenu;
        styleMenu.addItem(1, "Default (DAW Standard)", true, linkedAudio->getStyle() == WaveformStyle::Default);
        styleMenu.addItem(2, "Black Background", true, linkedAudio->getStyle() == WaveformStyle::BlackBackground);
        styleMenu.addItem(3, "Colored Background", true, linkedAudio->getStyle() == WaveformStyle::ColoredBackground);
        styleMenu.addItem(4, "No Background (Transparent)", true, linkedAudio->getStyle() == WaveformStyle::NoBackground);
        styleMenu.addItem(5, "Neon Glow", true, linkedAudio->getStyle() == WaveformStyle::NeonGlow);
        styleMenu.addItem(6, "Silhouette (Outline)", true, linkedAudio->getStyle() == WaveformStyle::Silhouette);
        styleMenu.addItem(7, "Vertical Gradient", true, linkedAudio->getStyle() == WaveformStyle::VerticalGradient);
        m.addSubMenu("Waveform Style", styleMenu);

        m.addSeparator();

        // --- SUBMENU: MODO DE VISTA (CONFIGURACIÓN DEL TRACK) ---
        Track* targetTrack = (cIdx < p.clips.size()) ? p.clips[cIdx].trackPtr : nullptr;
        if (targetTrack) {
            juce::PopupMenu viewMenu;
            const auto currentMode = targetTrack->getWaveformViewMode();
            viewMenu.addItem(10, "Vista: Combinada (L+R y Polaridad)", true, currentMode == WaveformViewMode::Combined);
            viewMenu.addItem(11, "Vista: Separada (L / R)", true, currentMode == WaveformViewMode::SeparateLR);
            viewMenu.addItem(12, "Vista: Mid / Side", true, currentMode == WaveformViewMode::MidSide);
            viewMenu.addItem(13, "Vista: Espectrograma (Frecuencias)", true, currentMode == WaveformViewMode::Spectrogram);
            m.addSubMenu("Waveform View", viewMenu);
        }

        m.addSeparator();
        m.addItem(50, "Show Waveform Diagnostics", true, AudioClip::isDebugInfoEnabled());
        m.addSeparator();
        m.addItem(100, "Eliminar Clip");

        // Mostrar menú asíncrono
        m.showMenuAsync(juce::PopupMenu::Options(), [linkedAudio, &p, cIdx](int result) {
            if (result == 0) return;

            Track* targetTrack = (cIdx < p.clips.size()) ? p.clips[cIdx].trackPtr : nullptr;

            if (result >= 1 && result <= 7) {
                linkedAudio->setStyle((WaveformStyle)(result - 1));
                if (targetTrack) targetTrack->commitSnapshot();
                p.repaint();
            }
            else if (result >= 10 && result <= 13) {
                if (targetTrack) {
                    targetTrack->setWaveformViewMode((WaveformViewMode)(result - 10));
                    targetTrack->commitSnapshot();
                    p.repaint();
                }
            }
            else if (result == 50) {
                AudioClip::setDebugInfoEnabled(!AudioClip::isDebugInfoEnabled());
                p.repaint();
            }
            else if (result == 100) {
                // Seleccionamos el clip si no lo está antes de borrar
                if (std::find(p.selectedClipIndices.begin(), p.selectedClipIndices.end(), cIdx) == p.selectedClipIndices.end()) {
                    p.selectedClipIndices.clear();
                    p.selectedClipIndices.push_back(cIdx);
                }
                p.deleteSelectedClips();
            }
        });
    }
};
