#include "ProjectManager.h"
#include "../PluginHost/VSTHost.h"
#include "../Native_Plugins/UtilityPlugin.h"
#include "../Modules/Routing/Data/RoutingData.h"
#include "../Modules/Routing/Data/SendData.h"

void ProjectManager::saveProject(TrackContainer& trackContainer, AudioEngine& engine, std::unique_ptr<juce::FileChooser>& fileChooser) {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Guardar Proyecto (.perritogordo)",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.perritogordo");

    auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [&trackContainer, &engine](const juce::FileChooser& fc) {
        auto file = fc.getResult();

        if (file != juce::File()) {
            if (file.getFileExtension() != ".perritogordo")
                file = file.withFileExtension(".perritogordo");

            juce::ValueTree projectTree("PERRITOGORDO_PROJECT");
            projectTree.setProperty("dawVersion", "1.0.0", nullptr);
            projectTree.setProperty("projectName", file.getFileNameWithoutExtension(), nullptr);

            juce::ValueTree settings("SETTINGS");
            settings.setProperty("bpm", (double)engine.transportState.bpm.load(), nullptr);
            projectTree.addChild(settings, -1, nullptr);

            juce::ValueTree tracksTree("TRACK_LIST");
            for (auto* track : trackContainer.getTracks()) {
                juce::ValueTree t("TRACK");
                t.setProperty("id", track->getId(), nullptr);
                t.setProperty("name", track->getName(), nullptr);
                t.setProperty("type", (int)track->getType(), nullptr);
                t.setProperty("vol", track->getVolume(), nullptr);
                t.setProperty("pan", track->getBalance(), nullptr);
                t.setProperty("color", track->getColor().toString(), nullptr);

                // --- PLUGINS ---
                juce::ValueTree pluginsTree("PLUGINS");
                for (auto* plugin : track->plugins) {
                    juce::ValueTree p("PLUGIN");
                    p.setProperty("name", plugin->getLoadedPluginName(), nullptr);
                    p.setProperty("isNative", plugin->isNative(), nullptr);
                    p.setProperty("bypassed", plugin->isBypassed(), nullptr);
                    p.setProperty("sidechainSource", (int)plugin->sidechain.sourceTrackId.load(), nullptr);

                    if (auto* v = dynamic_cast<VSTHost*>(plugin)) {
                        p.setProperty("vstPath", v->getPluginPath(), nullptr);
                    }

                    juce::MemoryBlock state;
                    plugin->getStateInformation(state);
                    if (state.getSize() > 0)
                        p.setProperty("base64State", state.toBase64Encoding(), nullptr);

                    pluginsTree.addChild(p, -1, nullptr);
                }
                t.addChild(pluginsTree, -1, nullptr);

                // --- SENDS (ENVÍOS) ---
                juce::ValueTree sendsTree("SENDS");
                for (const auto& send : track->routingData.sends) {
                    juce::ValueTree s("SEND");
                    s.setProperty("target", send.targetTrackId, nullptr);
                    s.setProperty("gain", send.gain, nullptr);
                    s.setProperty("isPre", send.isPreFader, nullptr);
                    s.setProperty("isMuted", send.isMuted, nullptr);
                    s.setProperty("routing", (int)send.mode, nullptr);
                    sendsTree.addChild(s, -1, nullptr);
                }
                t.addChild(sendsTree, -1, nullptr);

                // --- AUDIO CLIPS ---
                juce::ValueTree aClips("AUDIO_CLIPS");
                for (AudioClip* clip : track->getAudioClips()) {
                    juce::ValueTree c("CLIP");
                    c.setProperty("name", clip->getName(), nullptr);
                    c.setProperty("startX", (double)clip->getStartX(), nullptr);
                    c.setProperty("width", (double)clip->getWidth(), nullptr);
                    c.setProperty("offsetX", (double)clip->getOffsetX(), nullptr);
                    c.setProperty("path", clip->getSourceFilePath(), nullptr);
                    aClips.addChild(c, -1, nullptr);
                }
                t.addChild(aClips, -1, nullptr);

                // --- MIDI CLIPS ---
                juce::ValueTree mClips("MIDI_CLIPS");
                for (MidiPattern* clip : track->getMidiClips()) {
                    juce::ValueTree c("CLIP");
                    c.setProperty("name", clip->getName(), nullptr);
                    c.setProperty("startX", (double)clip->getStartX(), nullptr);
                    c.setProperty("width", (double)clip->getWidth(), nullptr);
                    c.setProperty("offsetX", (double)clip->getOffsetX(), nullptr);

                    juce::ValueTree notesTree("NOTES");
                    for (const Note& note : clip->getNotes()) {
                        juce::ValueTree n("N");
                        n.setProperty("p", note.pitch, nullptr);
                        n.setProperty("x", note.x, nullptr);
                        n.setProperty("w", note.width, nullptr);
                        notesTree.addChild(n, -1, nullptr);
                    }
                    c.addChild(notesTree, -1, nullptr);
                    mClips.addChild(c, -1, nullptr);
                }
                t.addChild(mClips, -1, nullptr);

                tracksTree.addChild(t, -1, nullptr);
            }
            projectTree.addChild(tracksTree, -1, nullptr);

            if (auto xml = projectTree.createXml()) {
                xml->writeTo(file);
            }
        }
    });
}

void ProjectManager::loadProject(const juce::File& file, TrackContainer& trackContainer, AudioEngine& engine, juce::CriticalSection* audioMutex, PlaylistComponent& playlistUI, EffectsPanel& effectsUI, PickerPanel& pickerUI, std::function<void()> onFinished) {
    if (!file.existsAsFile()) return;

    auto xml = juce::XmlDocument::parse(file);
    if (xml == nullptr) return;

    juce::ValueTree projectTree = juce::ValueTree::fromXml(*xml);
    if (!projectTree.hasType("PERRITOGORDO_PROJECT")) return;

    {
        juce::ScopedLock sl(*audioMutex);
        trackContainer.deselectAllTracks();
        while (trackContainer.getTracks().size() > 0) {
            trackContainer.removeTrack(0);
        }
    }

    // 1. Settings
    juce::ValueTree settings = projectTree.getChildWithName("SETTINGS");
    if (settings.isValid()) {
        double bpm = (double)settings.getProperty("bpm", 120.0);
        engine.transportState.bpm.store(bpm);
        playlistUI.setBpm(bpm);
    }

    // 2. Tracks
    juce::ValueTree tracksTree = projectTree.getChildWithName("TRACK_LIST");
    for (int i = 0; i < tracksTree.getNumChildren(); ++i) {
        juce::ValueTree tTree = tracksTree.getChild(i);
        
        int id = tTree.getProperty("id");
        TrackType tType = (TrackType)(int)tTree.getProperty("type");
        
        auto* t = trackContainer.addTrack(tType, id);
        if (t == nullptr) continue;

        t->setName(tTree.getProperty("name").toString());
        t->setVolume(tTree.getProperty("vol", 0.8f));
        t->setBalance(tTree.getProperty("pan", 0.0f));
        t->setColor(juce::Colour::fromString(tTree.getProperty("color").toString()));

        // --- SENDS (ENVÍOS) ---
        juce::ValueTree sList = tTree.getChildWithName("SENDS");
        for (int j = 0; j < sList.getNumChildren(); ++j) {
            juce::ValueTree sTree = sList.getChild(j);
            SendEntry s;
            s.targetTrackId = sTree.getProperty("target", -1);
            s.gain = sTree.getProperty("gain", 1.0f);
            s.isPreFader = sTree.getProperty("isPre", false);
            s.isMuted = sTree.getProperty("isMuted", false);
            s.mode = (RoutingMode)(int)sTree.getProperty("routing", 0);
            t->routingData.sends.push_back(s);
        }

        // --- AUDIO CLIPS ---
        juce::ValueTree aClips = tTree.getChildWithName("AUDIO_CLIPS");
        for (int j = 0; j < aClips.getNumChildren(); ++j) {
            juce::ValueTree cTree = aClips.getChild(j);
            juce::File sampleFile(cTree.getProperty("path").toString());
            if (sampleFile.existsAsFile()) {
                auto* clip = t->loadAndAddAudioClip(sampleFile, (float)cTree.getProperty("startX"));
                if (clip != nullptr) {
                    playlistUI.addAudioClipToView(t, clip);
                }
            }
        }

        // --- MIDI CLIPS ---
        juce::ValueTree mClips = tTree.getChildWithName("MIDI_CLIPS");
        for (int j = 0; j < mClips.getNumChildren(); ++j) {
            juce::ValueTree cTree = mClips.getChild(j);
            auto* clip = new MidiPattern();
            clip->setName(cTree.getProperty("name").toString());
            clip->setStartX((float)cTree.getProperty("startX"));
            clip->setWidth((float)cTree.getProperty("width"));
            clip->setOffsetX((float)cTree.getProperty("offsetX", 0.0f));
            
            juce::ValueTree notesTree = cTree.getChildWithName("NOTES");
            for (int k = 0; k < notesTree.getNumChildren(); ++k) {
                juce::ValueTree nTree = notesTree.getChild(k);
                clip->getNotes().push_back({ (int)nTree.getProperty("p"), (int)nTree.getProperty("x"), (int)nTree.getProperty("w") });
            }
            t->getMidiClips().add(clip);
            playlistUI.addMidiClipToView(t, clip);
        }

        // --- PLUGINS ---
        juce::ValueTree pTreeList = tTree.getChildWithName("PLUGINS");
        for (int j = 0; j < pTreeList.getNumChildren(); ++j) {
            juce::ValueTree pTree = pTreeList.getChild(j);
            bool isNative = pTree.getProperty("isNative");
            juce::String name = pTree.getProperty("name").toString();
            juce::String path = pTree.getProperty("vstPath").toString();
            juce::String state64 = pTree.getProperty("base64State").toString();
            int scSource = pTree.getProperty("sidechainSource", -1);

            if (isNative && name == "Utility") {
                effectsUI.onAddNativeUtility(*t);
            } else if (path.isNotEmpty() && effectsUI.onAddVST3FromFile) {
                effectsUI.onAddVST3FromFile(*t, path, state64, scSource);
            }
        }
        
        t->commitSnapshot();
    }
    
    // Recalcular topología global tras cargar todos los envíos
    engine.routingMatrix.commitNewTopology(trackContainer.getTracks());

    juce::MessageManager::callAsync([&playlistUI, &pickerUI, onFinished]() {
        playlistUI.updateScrollBars();
        playlistUI.repaint();
        pickerUI.refreshList();
        if (onFinished) onFinished();
    });
}