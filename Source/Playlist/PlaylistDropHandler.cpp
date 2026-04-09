#include "PlaylistDropHandler.h"
#include "PlaylistComponent.h"
#include "../Clips/Audio/AudioClip.h"
#include "../Clips/Midi/MidiPattern.h"

void PlaylistDropHandler::processExternalFiles(const juce::StringArray& files, int x, int y, PlaylistComponent& playlist) {
    int tIdx = playlist.getTrackAtY(y);

    bool isMidiFile = false;
    for (auto path : files) {
        if (path.endsWithIgnoreCase(".mid") || path.endsWithIgnoreCase(".midi")) {
            isMidiFile = true; break;
        }
    }

    TrackType requiredType = isMidiFile ? TrackType::MIDI : TrackType::Audio;

    if (tIdx != -1 && playlist.tracksRef && (*playlist.tracksRef)[tIdx]->getType() != requiredType) tIdx = -1;

    if (tIdx == -1 && playlist.onNewTrackRequested) {
        playlist.onNewTrackRequested(requiredType);
        if (playlist.tracksRef) tIdx = (int)playlist.tracksRef->size() - 1;
    }
    if (tIdx == -1) return;

    float absX = playlist.getAbsoluteXFromMouse(x);
    absX = std::round(absX / playlist.snapPixels) * playlist.snapPixels;
    absX = juce::jmax(0.0f, absX);

    if (!isMidiFile) {
        juce::AudioFormatManager fmt;
        fmt.registerBasicFormats();
        fmt.registerFormat(new juce::MP3AudioFormat(), false);

        for (auto path : files) {
            juce::File f(path);
            float clipWidth = 320.0f; // fallback, se actualiza con los metadatos del archivo
            if (f.existsAsFile()) {
                auto* data = new AudioClip();
                data->setName(f.getFileNameWithoutExtension());
                data->setStartX(absX);

                // Leer metadatos (duración) de forma inmediata y liviana
                // para calcular el ancho correcto sin cargar los samples
                {
                    juce::AudioFormatManager metaFmt;
                    metaFmt.registerBasicFormats();
                    std::unique_ptr<juce::AudioFormatReader> metaReader(metaFmt.createReaderFor(f));
                    if (metaReader != nullptr) {
                        double duration = (double)metaReader->lengthInSamples / metaReader->sampleRate;
                        clipWidth = (float)(duration * 160.0); // mismo factor que AudioClip::loadFromFile
                    }
                }
                data->setWidth(clipWidth);

                (*playlist.tracksRef)[tIdx]->getAudioClips().add(data);
                // Clip visible inmediatamente con ancho y nombre correctos, sin waveform
                playlist.clips.push_back({ (*playlist.tracksRef)[tIdx], absX, clipWidth, data->getName(), data, nullptr });

                auto trackPtr = (*playlist.tracksRef)[tIdx];
                playlist.repaint(); // Muestra el clip con "Cargando..." de inmediato

                // Carga completa (samples + peaks + espectrograma) en hilo de fondo
                juce::Thread::launch([&playlist, data, trackPtr, f]() {
                    data->loadFromFile(f, 44100.0);

                    juce::MessageManager::callAsync([&playlist, data, trackPtr]() {
                        for (auto& cv : playlist.clips) {
                            if (cv.linkedAudio == data) {
                                cv.width = data->getWidth();
                                break;
                            }
                        }
                        trackPtr->commitSnapshot();
                        playlist.updateScrollBars();
                        playlist.repaint();
                    });
                });
            }
            absX += clipWidth; // Usar el ancho real calculado para el siguiente clip
        }
    }
    else if (isMidiFile) {
        for (auto path : files) {
            juce::File f(path);
            if (f.existsAsFile() && (f.getFileExtension().equalsIgnoreCase(".mid") || f.getFileExtension().equalsIgnoreCase(".midi"))) {
                auto* pattern = new MidiPattern();
                pattern->setName(f.getFileNameWithoutExtension());
                pattern->setStartX(absX);
                pattern->setWidth(320.0f); // Ancho por defecto inicial
                
                // Nota: Aquí se debería parsear el archivo MIDI si quisiéramos importar notas reales.
                // Por ahora, creamos el "contenedor" de patrón como estaba previsto en el diseño.

                (*playlist.tracksRef)[tIdx]->getMidiClips().add(pattern);
                playlist.clips.push_back({ (*playlist.tracksRef)[tIdx], absX, 320.0f, pattern->getName(), nullptr, pattern });
                
                (*playlist.tracksRef)[tIdx]->commitSnapshot();
                absX += 320.0f;
            }
        }
    }
    playlist.updateScrollBars();
    playlist.repaint();
}

void PlaylistDropHandler::processInternalItem(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails, PlaylistComponent& playlist) {
    juce::String desc = dragSourceDetails.description.toString();

    if (desc == "FileBrowserDrag") {
        if (auto* tree = dynamic_cast<juce::FileTreeComponent*>(dragSourceDetails.sourceComponent.get())) {
            juce::File f = tree->getSelectedFile();
            if (f.existsAsFile()) {
                juce::StringArray arr;
                arr.add(f.getFullPathName());
                processExternalFiles(arr, dragSourceDetails.localPosition.x, dragSourceDetails.localPosition.y, playlist);
            }
        }
        return;
    }

    juce::StringArray tokens;
    tokens.addTokens(desc, "|", "");
    if (tokens.size() == 3) {
        bool isMidi = (tokens[1] == "MIDI");
        juce::String itemName = tokens[2];

        int x = dragSourceDetails.localPosition.x;
        int y = dragSourceDetails.localPosition.y;

        int tIdx = playlist.getTrackAtY(y);
        TrackType requiredType = isMidi ? TrackType::MIDI : TrackType::Audio;

        if (tIdx != -1 && playlist.tracksRef && (*playlist.tracksRef)[tIdx]->getType() != requiredType) tIdx = -1;

        if (tIdx == -1 && playlist.onNewTrackRequested) {
            playlist.onNewTrackRequested(requiredType);
            if (playlist.tracksRef && !playlist.tracksRef->isEmpty()) tIdx = playlist.tracksRef->size() - 1;
        }

        if (tIdx == -1) { playlist.repaint(); return; }

        Track* targetTrack = (*playlist.tracksRef)[tIdx];

        float absX = playlist.getAbsoluteXFromMouse(x);
        float snappedX = std::round(absX / playlist.snapPixels) * playlist.snapPixels;
        snappedX = juce::jmax(0.0f, snappedX);

        if (isMidi) {
            MidiPattern* sourceMidi = nullptr;
            for (auto* tr : *playlist.tracksRef) {
            for (MidiPattern* mc : tr->getMidiClips()) { if (mc->getName() == itemName) { sourceMidi = mc; break; } }
                if (sourceMidi) break;
            }
            if (!sourceMidi && playlist.trackContainer) {
                for (auto* mc : playlist.trackContainer->unusedMidiPool) { if (mc->getName() == itemName) { sourceMidi = mc; break; } }
            }

            if (sourceMidi) {
                MidiPattern* newMidiClip = new MidiPattern(*sourceMidi);
                float timeShift = snappedX - sourceMidi->getStartX();
                newMidiClip->setStartX(snappedX);
                for (auto& note : newMidiClip->getNotes()) note.x += timeShift;
                auto shiftAutoLane = [timeShift](AutoLane& lane) { for (auto& node : lane.nodes) node.x += timeShift; };
                shiftAutoLane(newMidiClip->autoVol);
                shiftAutoLane(newMidiClip->autoPan);
                shiftAutoLane(newMidiClip->autoPitch);
                shiftAutoLane(newMidiClip->autoFilter);
                targetTrack->getMidiClips().add(newMidiClip);
                playlist.clips.push_back({ targetTrack, snappedX, newMidiClip->getWidth(), newMidiClip->getName(), nullptr, newMidiClip });
                targetTrack->commitSnapshot(); // DOUBLE BUFFER: clip MIDI añadido por drop
                playlist.notifyPatternEdited(newMidiClip);
            }
        }
        else {
            AudioClip* sourceAudio = nullptr;
            for (auto* tr : *playlist.tracksRef) {
            for (AudioClip* ac : tr->getAudioClips()) { if (ac->getName() == itemName) { sourceAudio = ac; break; } }
                if (sourceAudio) break;
            }
            if (!sourceAudio && playlist.trackContainer) {
                for (auto* ac : playlist.trackContainer->unusedAudioPool) { if (ac->getName() == itemName) { sourceAudio = ac; break; } }
            }

            if (sourceAudio) {
                AudioClip* newAudioClip = new AudioClip(*sourceAudio);
                newAudioClip->setStartX(snappedX);
                targetTrack->getAudioClips().add(newAudioClip);
                playlist.clips.push_back({ targetTrack, snappedX, newAudioClip->getWidth(), newAudioClip->getName(), newAudioClip, nullptr });
                targetTrack->commitSnapshot(); // DOUBLE BUFFER: clip de audio añadido por drop
            }
        }
        playlist.updateScrollBars();
        playlist.repaint();
    }
}