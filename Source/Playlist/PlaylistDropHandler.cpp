#include "PlaylistDropHandler.h"
#include "PlaylistComponent.h" 

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
            std::unique_ptr<juce::AudioFormatReader> reader(fmt.createReaderFor(f));
            if (reader) {
                auto* data = new AudioClipData();
                data->name = f.getFileNameWithoutExtension();
                data->sourceFilePath = f.getFullPathName();
                data->startX = absX;

                // Configurar tamaño inicial vacío y calcular el ancho en píxeles.
                data->width = (float)(reader->lengthInSamples / reader->sampleRate) * (120.0f / 60.0f) * 80.0f;
                data->originalWidth = data->width;
                data->isLoaded.store(false); // Bandera para que el motor y UI sepan que está cargando

                (*playlist.tracksRef)[tIdx]->audioClips.add(data);
                playlist.clips.push_back({ (*playlist.tracksRef)[tIdx], absX, data->width, data->name, data, nullptr });
                (*playlist.tracksRef)[tIdx]->commitSnapshot(); // Compartir la caja vacía al audio thread
                absX += data->width;

                auto safePath = f.getFullPathName();
                auto trackPtr = (*playlist.tracksRef)[tIdx];

                juce::Thread::launch([data, safePath, &playlist, trackPtr]() {
                    juce::AudioFormatManager localFmt;
                    localFmt.registerBasicFormats();
                    localFmt.registerFormat(new juce::MP3AudioFormat(), false);
                    std::unique_ptr<juce::AudioFormatReader> localReader(localFmt.createReaderFor(juce::File(safePath)));
                    
                    if (localReader) {
                        data->fileBuffer.setSize((int)localReader->numChannels, (int)localReader->lengthInSamples);
                        localReader->read(&data->fileBuffer, 0, (int)localReader->lengthInSamples, 0, true, true);
                        data->generateCache();
                        data->isLoaded.store(true);

                        // Notificar a la UI en su propio hilo
                        juce::MessageManager::callAsync([trackPtr, &playlist]() {
                            trackPtr->commitSnapshot(); // Compartir el audio real ahora que está cargado
                            playlist.repaint();
                        });
                    }
                });
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
            MidiClipData* sourceMidi = nullptr;
            for (auto* tr : *playlist.tracksRef) {
                for (auto* mc : tr->midiClips) { if (mc->name == itemName) { sourceMidi = mc; break; } }
                if (sourceMidi) break;
            }
            if (!sourceMidi && playlist.trackContainer) {
                for (auto* mc : playlist.trackContainer->unusedMidiPool) { if (mc->name == itemName) { sourceMidi = mc; break; } }
            }

            if (sourceMidi) {
                MidiClipData* newMidiClip = new MidiClipData(*sourceMidi);
                float timeShift = snappedX - sourceMidi->startX;
                newMidiClip->startX = snappedX;
                for (auto& note : newMidiClip->notes) note.x += timeShift;
                auto shiftAutoLane = [timeShift](AutoLane& lane) { for (auto& node : lane.nodes) node.x += timeShift; };
                shiftAutoLane(newMidiClip->autoVol);
                shiftAutoLane(newMidiClip->autoPan);
                shiftAutoLane(newMidiClip->autoPitch);
                shiftAutoLane(newMidiClip->autoFilter);
                targetTrack->midiClips.add(newMidiClip);
                playlist.clips.push_back({ targetTrack, snappedX, newMidiClip->width, newMidiClip->name, nullptr, newMidiClip });
                targetTrack->commitSnapshot(); // DOUBLE BUFFER: clip MIDI añadido por drop
                playlist.notifyPatternEdited(newMidiClip);
            }
        }
        else {
            AudioClipData* sourceAudio = nullptr;
            for (auto* tr : *playlist.tracksRef) {
                for (auto* ac : tr->audioClips) { if (ac->name == itemName) { sourceAudio = ac; break; } }
                if (sourceAudio) break;
            }
            if (!sourceAudio && playlist.trackContainer) {
                for (auto* ac : playlist.trackContainer->unusedAudioPool) { if (ac->name == itemName) { sourceAudio = ac; break; } }
            }

            if (sourceAudio) {
                AudioClipData* newAudioClip = new AudioClipData();
                newAudioClip->name = sourceAudio->name;
                newAudioClip->startX = snappedX;
                newAudioClip->width = sourceAudio->width;
                newAudioClip->sourceSampleRate = sourceAudio->sourceSampleRate;
                newAudioClip->fileBuffer.makeCopyOf(sourceAudio->fileBuffer);
                newAudioClip->sourceFilePath = sourceAudio->sourceFilePath;
                newAudioClip->cachedPeaksL = sourceAudio->cachedPeaksL;
                newAudioClip->cachedPeaksR = sourceAudio->cachedPeaksR;
                newAudioClip->cachedPeaksMid = sourceAudio->cachedPeaksMid;
                newAudioClip->cachedPeaksSide = sourceAudio->cachedPeaksSide;
                targetTrack->audioClips.add(newAudioClip);
                playlist.clips.push_back({ targetTrack, snappedX, newAudioClip->width, newAudioClip->name, newAudioClip, nullptr });
                targetTrack->commitSnapshot(); // DOUBLE BUFFER: clip de audio añadido por drop
            }
        }
        playlist.updateScrollBars();
        playlist.repaint();
    }
}