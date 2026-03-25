#pragma once
#include <JuceHeader.h>
#include "Tracks/TrackContainer.h"
#include "Playlist/PlaylistComponent.h"

class ProjectManager 
{
public:
    // --- FUNCIÓN PROFESIONAL DE GUARDADO (BINARIO) ---
    static void save(const juce::File& file, TrackContainer& container)
    {
        juce::ValueTree projectTree("PERRITOGORDO_PROJECT");
        projectTree.setProperty("dawVersion", ProjectInfo::versionString, nullptr);

        juce::ValueTree tracksTree("TRACK_LIST");
        
        for (auto* track : container.getTracks())
        {
            juce::ValueTree t("TRACK");
            t.setProperty("name", track->getName(), nullptr);
            t.setProperty("type", (int)track->getType(), nullptr);

            juce::ValueTree clipsNode("CLIPS");

            // Guardar Clips MIDI y sus Notas
            for (auto* mc : track->midiClips)
            {
                juce::ValueTree mcTree("MIDI_CLIP");
                mcTree.setProperty("name", mc->name, nullptr);
                mcTree.setProperty("startX", mc->startX, nullptr);
                mcTree.setProperty("width", mc->width, nullptr);

                juce::ValueTree notesNode("NOTES");
                for (auto& note : mc->notes)
                {
                    juce::ValueTree n("NOTE");
                    n.setProperty("x", note.x, nullptr);
                    n.setProperty("y", note.y, nullptr);
                    n.setProperty("w", note.w, nullptr);
                    notesNode.addChild(n, -1, nullptr);
                }
                mcTree.addChild(notesNode, -1, nullptr);
                clipsNode.addChild(mcTree, -1, nullptr);
            }

            // Guardar Clips de Audio (Rutas de archivos)
            for (auto* ac : track->audioClips)
            {
                juce::ValueTree acTree("AUDIO_CLIP");
                acTree.setProperty("name", ac->name, nullptr);
                acTree.setProperty("startX", ac->startX, nullptr);
                acTree.setProperty("width", ac->width, nullptr);
                acTree.setProperty("filePath", ac->sourceFilePath, nullptr); // <-- RUTA REAL DEL SAMPLE
                clipsNode.addChild(acTree, -1, nullptr);
            }

            t.addChild(clipsNode, -1, nullptr);
            tracksTree.addChild(t, -1, nullptr);
        }
        projectTree.addChild(tracksTree, -1, nullptr);

        juce::FileOutputStream outputStream(file);
        if (outputStream.openedOk())
        {
            outputStream.setPosition(0);
            outputStream.truncate();
            projectTree.writeToStream(outputStream);
        }
    }

    // --- FUNCIÓN PROFESIONAL DE CARGA ---
    static void load(const juce::File& file, TrackContainer& container, PlaylistComponent& playlist, juce::CriticalSection& audioMutex)
    {
        juce::FileInputStream inputStream(file);
        if (!inputStream.openedOk()) return;

        juce::ValueTree projectTree = juce::ValueTree::readFromStream(inputStream);
        if (!projectTree.isValid() || !projectTree.hasType("PERRITOGORDO_PROJECT")) return;

        const juce::ScopedLock sl(audioMutex);
        
        // Limpiamos todo
        playlist.clips.clear();
        while (container.getTracks().size() > 0) container.removeTrack(0);

        juce::ValueTree tracksTree = projectTree.getChildWithName("TRACK_LIST");
        if (tracksTree.isValid())
        {
            for (auto tTree : tracksTree)
            {
                TrackType type = (TrackType)(int)tTree.getProperty("type");
                container.addTrack(type);
                auto* newTrack = container.getTracks().getLast();
                newTrack->setName(tTree.getProperty("name").toString());

                juce::ValueTree clipsTree = tTree.getChildWithName("CLIPS");
                for (auto cTree : clipsTree)
                {
                    if (cTree.hasType("MIDI_CLIP"))
                    {
                        auto* midi = new MidiClipData();
                        midi->name = cTree.getProperty("name");
                        midi->startX = cTree.getProperty("startX");
                        midi->width = cTree.getProperty("width");
                        
                        juce::ValueTree notesTree = cTree.getChildWithName("NOTES");
                        for (auto nTree : notesTree)
                        {
                            MidiNote n;
                            n.x = nTree.getProperty("x");
                            n.y = nTree.getProperty("y");
                            n.w = nTree.getProperty("w");
                            midi->notes.push_back(n);
                        }
                        newTrack->midiClips.add(midi);
                        playlist.addMidiClipToView(newTrack, midi);
                    }
                    else if (cTree.hasType("AUDIO_CLIP"))
                    {
                        juce::File sampleFile(cTree.getProperty("filePath").toString());
                        if (sampleFile.existsAsFile())
                        {
                            juce::AudioFormatManager fmt;
                            fmt.registerBasicFormats();
                            fmt.registerFormat(new juce::MP3AudioFormat(), false);
                            
                            if (auto reader = std::unique_ptr<juce::AudioFormatReader>(fmt.createReaderFor(sampleFile)))
                            {
                                auto* audio = new AudioClipData();
                                audio->name = cTree.getProperty("name");
                                audio->startX = cTree.getProperty("startX");
                                audio->sourceFilePath = sampleFile.getFullPathName();
                                audio->fileBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
                                reader->read(&audio->fileBuffer, 0, (int)reader->lengthInSamples, 0, true, true);
                                audio->generateCache();
                                audio->width = cTree.getProperty("width");
                                
                                newTrack->audioClips.add(audio);
                                playlist.clips.push_back({ newTrack, audio->startX, audio->width, audio->name, audio, nullptr });
                            }
                        }
                    }
                }
            }
        }
    }
};