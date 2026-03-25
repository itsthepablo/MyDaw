#include "ProjectManager.h"

void ProjectManager::saveProject(TrackContainer& trackContainer, std::unique_ptr<juce::FileChooser>& fileChooser) {
    fileChooser = std::make_unique<juce::FileChooser>(
        "Guardar Proyecto (.perritogordo)",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory),
        "*.perritogordo");

    auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [&trackContainer](const juce::FileChooser& fc) {
        auto file = fc.getResult();

        if (file != juce::File()) {
            if (file.getFileExtension() != ".perritogordo")
                file = file.withFileExtension(".perritogordo");

            juce::ValueTree projectTree("PERRITOGORDO_PROJECT");
            projectTree.setProperty("dawVersion", ProjectInfo::versionString, nullptr);
            projectTree.setProperty("projectName", file.getFileNameWithoutExtension(), nullptr);

            juce::ValueTree settings("SETTINGS");
            settings.setProperty("bpm", 120.0, nullptr);
            projectTree.addChild(settings, -1, nullptr);

            juce::ValueTree tracksTree("TRACK_LIST");
            for (auto* track : trackContainer.getTracks()) {
                juce::ValueTree t("TRACK");
                t.setProperty("name", track->getName(), nullptr);
                tracksTree.addChild(t, -1, nullptr);
            }
            projectTree.addChild(tracksTree, -1, nullptr);

            if (auto xml = projectTree.createXml()) {
                xml->writeTo(file);
            }
        }
    });
}

void ProjectManager::loadProject(const juce::File& file, 
                                TrackContainer& trackContainer, 
                                juce::CriticalSection& audioMutex,
                                PlaylistComponent& playlistUI,
                                PickerPanel& pickerPanelUI,
                                std::function<void()> onFinished) {
    if (!file.existsAsFile()) return;

    auto xml = juce::XmlDocument::parse(file);
    if (xml != nullptr) {
        if (xml->hasTagName("PERRITOGORDO_PROJECT")) {
            const juce::ScopedLock sl(audioMutex);

            while (trackContainer.getTracks().size() > 0) {
                trackContainer.removeTrack(0);
            }

            if (auto* tracksTree = xml->getChildByName("TRACK_LIST")) {
                for (auto* tXml : tracksTree->getChildIterator()) {
                    if (tXml->hasTagName("TRACK")) {
                        trackContainer.addTrack(TrackType::Audio);
                    }
                }
            }

            playlistUI.updateScrollBars();
            playlistUI.repaint();
            pickerPanelUI.refreshList();
            if (onFinished) onFinished();
        }
    }
}