#pragma once
#include <JuceHeader.h>
#include "Tracks/UI/TrackContainer.h"
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
                mcTree.setProperty("name", mc->getName(), nullptr);
                mcTree.setProperty("startX", mc->getStartX(), nullptr);
                mcTree.setProperty("width", mc->getWidth(), nullptr);
                mcTree.setProperty("style", (int)mc->getStyle(), nullptr);

                juce::ValueTree notesNode("NOTES");
                for (auto& note : mc->getNotes())
                {
                    juce::ValueTree n("NOTE");
                    n.setProperty("x", note.x, nullptr);
                    n.setProperty("pitch", note.pitch, nullptr);
                    n.setProperty("width", note.width, nullptr);
                    notesNode.addChild(n, -1, nullptr);
                }
                mcTree.addChild(notesNode, -1, nullptr);
                clipsNode.addChild(mcTree, -1, nullptr);
            }

            // Guardar Clips de Audio (Rutas de archivos)
            for (auto* ac : track->audioClips)
            {
                juce::ValueTree acTree("AUDIO_CLIP");
                acTree.setProperty("name", ac->getName(), nullptr);
                acTree.setProperty("startX", ac->getStartX(), nullptr);
                acTree.setProperty("width", ac->getWidth(), nullptr);
                acTree.setProperty("filePath", ac->getSourceFilePath(), nullptr); // <-- RUTA REAL DEL SAMPLE
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
                        auto* midi = new MidiPattern();
                        midi->setName(cTree.getProperty("name"));
                        midi->setStartX(cTree.getProperty("startX"));
                        midi->setWidth(cTree.getProperty("width"));
                        midi->setStyle((MidiStyleType)(int)cTree.getProperty("style", 0));

                        juce::ValueTree notesTree = cTree.getChildWithName("NOTES");
                        for (auto nTree : notesTree)
                        {
                            Note n;
                            n.x = nTree.getProperty("x");
                            n.pitch = nTree.getProperty("pitch");
                            n.width = nTree.getProperty("width");
                            n.frequency = 0; // Default frequency
                            midi->getNotes().push_back(n);
                        }
                        newTrack->midiClips.add(midi);
                        playlist.addMidiClipToView(newTrack, midi);
                    }
                    else if (cTree.hasType("AUDIO_CLIP"))
                    {
                        juce::File sampleFile(cTree.getProperty("filePath").toString());
                        if (sampleFile.existsAsFile())
                        {
                            auto* audio = new AudioClip();
                            audio->setStartX(cTree.getProperty("startX"));
                            if (audio->loadFromFile(sampleFile, 44100.0))
                            {
                                audio->setName(cTree.getProperty("name"));
                                audio->setWidth(cTree.getProperty("width"));
                                newTrack->audioClips.add(audio);
                                playlist.clips.push_back({ newTrack, audio->getStartX(), audio->getWidth(), audio->getName(), audio, nullptr });
                            }
                            else
                            {
                                delete audio;
                            }
                        }
                    }
                }
            }
        }
    }
};