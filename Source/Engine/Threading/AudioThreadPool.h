#pragma once
#include <JuceHeader.h>
#include <atomic>
#include <thread>
#include <vector>
#include "../Core/AudioClock.h"
#include "../Routing/RoutingMatrix.h"
#include "../Routing/MasterMixer.h"
#include "../Routing/PDCManager.h"
#include "../Nodes/TrackProcessor.h"

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

    void prepare()
    {
        if (prepared.exchange(true)) return;
        int hw = (int)std::thread::hardware_concurrency();
        numWorkers = juce::jlimit(0, 15, hw - 1);
        shouldStop.store(false, std::memory_order_relaxed);
        workerThreads.reserve(numWorkers);
        for (int i = 0; i < numWorkers; ++i)
            workerThreads.emplace_back([this] { workerLoop(); });
    }

    void processTracksParallel(
        const TopoState* topo,
        const AudioClock& clock,
        int numSamples,
        bool isPlayingNow,
        bool isStoppingNow,
        const juce::MidiBuffer& previewMidi,
        int hardwareOutChannels) noexcept
    {
        int n = (int)topo->activeTracks.size();
        if (n == 0) return;

        ctx.topo          = topo;
        ctx.clock         = &clock;
        ctx.previewMidi   = &previewMidi;
        ctx.numSamples    = numSamples;
        ctx.hwOutChannels = hardwareOutChannels;
        ctx.numTasks      = n;
        ctx.isPlayingNow  = isPlayingNow;
        ctx.isStoppingNow = isStoppingNow;

        for (int i = 0; i < n; ++i) {
            // En los DAWs comerciales modernos (Reaper, FL, Ableton), diferentes instancias 
            // de VST3 corren en diferentes hilos libremente. El grafo garantiza que nunca 
            // se procese la misma pista 2 veces simultáneamente.
            mainThreadTask[i] = false;

            int pending = (i < (int)topo->initialPendingCounts.size())
                          ? topo->initialPendingCounts[i] : 0;
            pendingChildren[i].store(pending, std::memory_order_relaxed);
            TaskState init = (pending == 0) ? TaskState::Ready : TaskState::Waiting;
            taskStates[i].store((int)init, std::memory_order_relaxed);
        }
        doneCount.store(0, std::memory_order_release);
        generation.fetch_add(1, std::memory_order_release); // Despertar workers

        // Audio thread: procesa TODOS los tasks (incluidos los de plugins)
        // Workers: solo procesa tasks sin plugins (mainThreadTask == false)
        while (doneCount.load(std::memory_order_acquire) < n) {
            stealOneTask(/*isAudioThread=*/true);
            // Spin-Wait robusto: No le damos el control al OS para evitar dropouts de 15ms.
            for (int i = 0; i < 40; ++i) std::atomic_thread_fence(std::memory_order_acquire);
        }
    }

    void shutdown()
    {
        if (!prepared.load(std::memory_order_acquire)) return;
        shouldStop.store(true, std::memory_order_release);
        generation.fetch_add(0xFFFF, std::memory_order_release);
        for (auto& t : workerThreads)
            if (t.joinable()) t.join();
        workerThreads.clear();
        prepared.store(false, std::memory_order_relaxed);
    }

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
    };

    // -------------------------------------------------------------------------
    // Intenta robar y ejecutar UN task listo.
    // isAudioThread=true: puede tomar cualquier task (incluidos los de plugins).
    // isAudioThread=false: salta los tasks que deben correr en el audio thread.
    // -------------------------------------------------------------------------
    void stealOneTask(bool isAudioThread) noexcept
    {
        int n = ctx.numTasks;
        for (int i = 0; i < n; ++i) {
            if (!isAudioThread && mainThreadTask[i]) continue; // Workers saltan plugins
            int expected = (int)TaskState::Ready;
            if (taskStates[i].compare_exchange_strong(
                    expected, (int)TaskState::InProgress,
                    std::memory_order_acq_rel, std::memory_order_relaxed)) {
                processOneTask(i);
                taskStates[i].store((int)TaskState::Done, std::memory_order_release);
                return;
            }
        }
    }

    void processOneTask(int idx) noexcept
    {
        juce::ScopedNoDenormals noDenormals; // [PROTECCIÓN OBLIGATORIA] CPU sin picos por flotación

        const TopoState* topo = ctx.topo;
        if (idx < 0 || idx >= ctx.numTasks) return;
        auto* track = topo->activeTracks[idx];

        // 1. Acumular audio de hijos ya procesados
        if (idx < (int)topo->childrenOf.size()) {
            for (int childIdx : topo->childrenOf[idx]) {
                auto* child = topo->activeTracks[childIdx];
                int chans = juce::jmin(track->audioBuffer.getNumChannels(),
                                        child->audioBuffer.getNumChannels());
                for (int c = 0; c < chans; ++c)
                    track->audioBuffer.addFrom(c, 0, child->audioBuffer, c, 0, ctx.numSamples);
            }
        }

        // 2. DSP principal
        TrackProcessor::process(track, *ctx.clock, ctx.numSamples,
                                ctx.isPlayingNow, ctx.isStoppingNow, *ctx.previewMidi);

        // 3. Compensacion de latencia
        PDCManager::applyDelay(track, ctx.numSamples);

        // 4. Volumen y pan (resultado queda en track->audioBuffer para Fase 2)
        MasterMixer::applyGainAndPan(track, ctx.numSamples, ctx.hwOutChannels);

        // 5. Notificar al padre
        int parentIdx = (idx < (int)topo->parentIndices.size())
                        ? topo->parentIndices[idx] : -1;
        if (parentIdx >= 0 && parentIdx < ctx.numTasks) {
            int remaining = pendingChildren[parentIdx].fetch_sub(1, std::memory_order_acq_rel) - 1;
            if (remaining == 0)
                taskStates[parentIdx].store((int)TaskState::Ready, std::memory_order_release);
        }

        doneCount.fetch_add(1, std::memory_order_release);
    }

    void workerLoop() noexcept
    {
        int lastGen = 0;
        int spinIter = 0;
        while (!shouldStop.load(std::memory_order_acquire)) {
            int gen = generation.load(std::memory_order_acquire);
            if (gen != lastGen) {
                lastGen = gen;
                spinIter = 0;
                if (shouldStop.load(std::memory_order_relaxed)) break;
                // Workers procesan solo tasks sin plugins
                while (doneCount.load(std::memory_order_acquire) < ctx.numTasks) {
                    stealOneTask(/*isAudioThread=*/false);
                    for (int i = 0; i < 40; ++i) std::atomic_thread_fence(std::memory_order_acquire);
                }
            } else {
                if (++spinIter < 2000)
                    std::atomic_thread_fence(std::memory_order_acquire);
                else { spinIter = 0; std::this_thread::yield(); } // Solo cede cuando está 100% IDLE
            }
        }
    }

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
