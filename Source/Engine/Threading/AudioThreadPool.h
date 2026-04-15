#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <thread>
#include <vector>
#include <map>
#include "../Core/AudioClock.h"
#include "../Routing/RoutingMatrix.h"
#include "../Routing/PDCManager.h"
#include "../Nodes/TrackProcessor.h"
#include "../../Mixer/DSP/MixerDSP.h"

using TopoState = RoutingMatrix::TopoState;
enum class TaskState : int { Waiting = 0, Ready = 1, InProgress = 2, Done = 3 };

// ==============================================================================
// AudioThreadPool — Real-Time Dependency Task Graph
//
// Los tracks con plugins VST3 se procesan SOLO en el audio thread principal
// (requerimiento del spec VST3). Los tracks sin plugins se reparten entre
// todos los workers para maxima paralalizacion.
// ==============================================================================
class AudioThreadPool {
public:
    AudioThreadPool() = default;
    ~AudioThreadPool() { shutdown(); }

    void prepare();

    void processTracksParallel(
        const TopoState* topo,
        const AudioClock& clock,
        int numSamples,
        bool isPlayingNow,
        bool isStoppingNow,
        const juce::MidiBuffer& previewMidi,
        int hardwareOutChannels) noexcept;

    void shutdown();

    int getNumWorkers()   const noexcept { return numWorkers; }

private:
    static constexpr int MAX_TRACKS = 128;

    struct alignas(64) BlockContext {
        const TopoState*        topo          = nullptr;
        const AudioClock*       clock         = nullptr;
        const juce::MidiBuffer* previewMidi   = nullptr;
        int  numSamples      = 0;
        int  hwOutChannels   = 2;
        int  numTasks        = 0;
        bool isPlayingNow    = false;
        bool isStoppingNow   = false;
        std::map<int, int> idToIndex; // Auxiliar rápido
        juce::AudioBuffer<float> msWorkBuffer; // Buffer temporal para cálculos Mid/Side
    };

    void stealOneTask(bool isAudioThread) noexcept;
    void processOneTask(int idx) noexcept;
    void workerLoop() noexcept;

    BlockContext ctx;
    bool mainThreadTask[MAX_TRACKS] = {};

    alignas(64) std::atomic<int> generation    { 0 };
    alignas(64) std::atomic<int> doneCount     { 0 };
    std::atomic<int> taskStates     [MAX_TRACKS] = {};
    std::atomic<int> pendingChildren[MAX_TRACKS] = {};

    std::vector<std::thread> workerThreads;
    std::atomic<bool> shouldStop { false };
    std::atomic<bool> prepared   { false };
    int numWorkers = 0;

    JUCE_DECLARE_NON_COPYABLE(AudioThreadPool)
};
