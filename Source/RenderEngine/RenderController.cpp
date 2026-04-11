#include "RenderController.h"
#include "OfflineRenderer.h"
#include "StemRenderer/StemRendererWindow.h"
#include "../UI/UIManager.h"
#include "../Engine/Core/AudioEngine.h"

RenderController::RenderController(DAWUIComponents& ui,
                                   AudioEngine& audioEngine,
                                   juce::AudioAppComponent& appComponent,
                                   juce::CriticalSection& audioMutex,
                                   std::atomic<bool>& isOfflineRendering)
    : ui(ui),
      audioEngine(audioEngine),
      appComponent(appComponent),
      audioMutex(audioMutex),
      isOfflineRendering(isOfflineRendering)
{}

RenderController::~RenderController() = default;

void RenderController::startExport(juce::Component* parentComponent)
{
    if (isOfflineRendering.load()) return;

    if (audioEngine.transportState.isPlaying.load())
        ui.topMenuBar.stopBtn.triggerClick();

    ensureOfflineRenderer(parentComponent);

    offlineRenderer->onProcessOfflineBlock = [this](juce::AudioBuffer<float>& buffer, int numSamples) {
        juce::AudioSourceChannelInfo info(&buffer, 0, numSamples);
        audioEngine.processBlock(info);
    };

    offlineRenderer->onPrepareEngine = [this](double sampleRate) {
        audioEngine.prepareToPlay(4096, sampleRate);
        audioEngine.resetForRender();
        const juce::ScopedLock sl(audioMutex);
        for (auto* t : ui.trackContainer.getTracks())
            t->prepare(sampleRate, 4096);
        if (ui.masterTrackObj)
            ui.masterTrackObj->prepare(sampleRate, 4096);
    };

    offlineRenderer->onClose = [this] {
        audioEngine.transportState.isPlaying.store(false);
        audioEngine.setNonRealtime(false);
        double hwSampleRate = appComponent.deviceManager.getAudioDeviceSetup().sampleRate;
        audioEngine.prepareToPlay(512, hwSampleRate);
        const juce::ScopedLock sl(audioMutex);
        for (auto* t : ui.trackContainer.getTracks())
            t->prepare(hwSampleRate, 512);
        if (ui.masterTrackObj)
            ui.masterTrackObj->prepare(hwSampleRate, 512);
        audioEngine.resetForRender();
        offlineRenderer->setVisible(false);
        isOfflineRendering.store(false);
    };

    isOfflineRendering.store(true);
    audioEngine.setNonRealtime(true);
    audioEngine.resetForRender();
    audioEngine.transportState.isPlaying.store(true);
    audioEngine.transportState.loopEndPos.store(
        std::numeric_limits<float>::max(), std::memory_order_relaxed);

    double currentBpm = ui.playlistUI.getBpm();
    double timelinePixels = ui.playlistUI.getTimelineLength();
    double lengthSecs = (timelinePixels / 320.0) * 4.0 * (60.0 / currentBpm);

    offlineRenderer->setBounds(parentComponent->getLocalBounds().withSizeKeepingCentre(700, 500));
    offlineRenderer->setVisible(true);
    offlineRenderer->toFront(true);
    offlineRenderer->showConfig(lengthSecs);
}

void RenderController::showStemRenderer(juce::Component* parentComponent)
{
    std::vector<TrackStemInfo> stems;
    for (auto* t : ui.trackContainer.getTracks()) {
        if (t->getType() == TrackType::Loudness ||
            t->getType() == TrackType::Balance  ||
            t->getType() == TrackType::MidSide)
            continue;

        TrackStemInfo info;
        info.id          = t->getId();
        info.name        = t->getName();
        info.parentId    = t->parentId;
        info.type        = t->getType();
        info.folderDepth = t->folderDepth;
        info.selected    = true;
        stems.push_back(info);
    }

    stemRendererWindow = std::make_unique<StemRendererHost>(stems);

    stemRendererWindow->onExportStarts = [this, parentComponent](std::vector<int> tracks) {
        if (tracks.empty()) return;

        stemRendererWindow->setVisible(false);

        preRenderMuteStates.clear();
        preRenderSoloStates.clear();
        for (auto* t : ui.trackContainer.getTracks()) {
            preRenderMuteStates[t->getId()] = t->mixerData.isMuted.load();
            preRenderSoloStates[t->getId()] = t->mixerData.isSoloed.load();
        }

        ensureOfflineRenderer(parentComponent);

        offlineRenderer->onClose = [this] {
            offlineRenderer->setVisible(false);
            if (isOfflineRendering.load()) {
                isOfflineRendering.store(false);
                audioEngine.setNonRealtime(false);
                audioEngine.transportState.isPlaying.store(false);
                audioEngine.prepareToPlay(512, appComponent.deviceManager.getAudioDeviceSetup().sampleRate);
                audioEngine.resetForRender();
            }
        };

        offlineRenderer->onGetTrackName = [this](int trackId) -> juce::String {
            for (auto* t : ui.trackContainer.getTracks()) {
                if (t->getId() == trackId) {
                    return t->getName()
                        .replaceCharacter(' ', '_')
                        .retainCharacters("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-");
                }
            }
            return "Track_" + juce::String(trackId);
        };

        offlineRenderer->onPrepareStemTrack = [this](int trackId) {
            const juce::ScopedLock sl(audioMutex);
            auto& allTracks = ui.trackContainer.getTracks();

            for (auto* t : allTracks) {
                t->mixerData.isMuted.store(true);
                t->mixerData.isSoloed.store(false);
            }

            Track* target = nullptr;
            for (auto* t : allTracks) { if (t->getId() == trackId) target = t; }
            if (!target) return;

            target->mixerData.isMuted.store(false);

            if (target->getType() == TrackType::Folder) {
                bool changed = true;
                while (changed) {
                    changed = false;
                    for (auto* child : allTracks) {
                        if (child->mixerData.isMuted.load()) {
                            for (auto* p : allTracks) {
                                if (p->getId() == child->parentId &&
                                    !p->mixerData.isMuted.load() &&
                                    p->getType() == TrackType::Folder) {
                                    child->mixerData.isMuted.store(false);
                                    changed = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            } else {
                int currParentId = target->parentId;
                while (currParentId != -1) {
                    bool found = false;
                    for (auto* t : allTracks) {
                        if (t->getId() == currParentId) {
                            t->mixerData.isMuted.store(false);
                            currParentId = t->parentId;
                            found = true;
                            break;
                        }
                    }
                    if (!found) break;
                }
            }
        };

        offlineRenderer->onPrepareEngine = [this](double sampleRate) {
            audioEngine.prepareToPlay(4096, sampleRate);
            audioEngine.resetForRender();
            for (auto* t : ui.trackContainer.getTracks())
                t->prepare(sampleRate, 4096);
            if (ui.masterTrackObj)
                ui.masterTrackObj->prepare(sampleRate, 4096);
        };

        offlineRenderer->onProcessOfflineBlock = [this](juce::AudioBuffer<float>& buffer, int numSamples) {
            juce::AudioSourceChannelInfo info(&buffer, 0, numSamples);
            audioEngine.processBlock(info);
        };

        offlineRenderer->engine->onProcessOfflineBlock = offlineRenderer->onProcessOfflineBlock;
        offlineRenderer->engine->onPrepareEngine       = offlineRenderer->onPrepareEngine;
        offlineRenderer->setPendingStems(tracks);

        double currentBpm    = ui.playlistUI.getBpm();
        double timelinePixels = ui.playlistUI.getTimelineLength();
        double lengthSecs    = (timelinePixels / 320.0) * 4.0 * (60.0 / currentBpm);

        isOfflineRendering.store(true);
        audioEngine.setNonRealtime(true);
        audioEngine.resetForRender();
        audioEngine.transportState.isPlaying.store(true);
        audioEngine.transportState.loopEndPos.store(
            std::numeric_limits<float>::max(), std::memory_order_relaxed);

        offlineRenderer->setBounds(parentComponent->getLocalBounds().withSizeKeepingCentre(700, 500));
        offlineRenderer->toFront(true);
        offlineRenderer->setVisible(true);
        offlineRenderer->showConfig(lengthSecs);
    };

    stemRendererWindow->setVisible(true);
    stemRendererWindow->toFront(true);
}

void RenderController::ensureOfflineRenderer(juce::Component* parent)
{
    if (!offlineRenderer) {
        offlineRenderer = std::make_unique<OfflineRenderer>();
        parent->addChildComponent(offlineRenderer.get());
    }
}
