#pragma once
#include <JuceHeader.h>
#include <functional>
#include "../NativePlugins/BaseEffect.h"
#include "../Modules/Routing/UI/SidechainSelector.h"
#include "../Engine/Modulation/ModulationTargets.h"
#include "../Engine/Modulation/GridModulator.h"

// Forward declarations
class TrackContainer;
class VSTHost;

/**
 * DawPlayHead: El reloj interno del host para sincronizar VSTs.
 */
class DawPlayHead : public juce::AudioPlayHead {
public:
    juce::Optional<PositionInfo> getPosition() const override {
        juce::AudioPlayHead::PositionInfo info;
        info.setIsPlaying(isPlaying);
        info.setTimeInSamples(currentSample);
        info.setBpm(120.0);
        return info;
    }

    bool isPlaying = false;
    int64_t currentSample = 0;
};

class VSTCustomHeader : public juce::Component {
public:
    VSTCustomHeader(juce::DocumentWindow* w, BaseEffect* fx, TrackContainer* container, std::function<void()> onUpdate);
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override { dragger.startDraggingComponent(window, e); }
    void mouseDrag(const juce::MouseEvent& e) override { dragger.dragComponent(window, e, nullptr); }
private:
    juce::DocumentWindow* window;
    BaseEffect* effect;
    TrackContainer* trackContainer;
    juce::TextButton closeBtn;
    SidechainSelector scSelector;
    juce::ComponentDragger dragger;
    std::function<void()> onTopologyUpdate;
};

class VSTContainer : public juce::Component, private juce::ComponentListener {
public:
    VSTContainer(juce::DocumentWindow* w, juce::AudioProcessorEditor* ed, BaseEffect* fx, TrackContainer* container, std::function<void()> onUpdate) 
        : header(w, fx, container, onUpdate), editor(ed) 
    {
        addAndMakeVisible(editor.get());
        addAndMakeVisible(header);
        editor->addComponentListener(this);
        updateSize();
    }
    ~VSTContainer() override { if (editor != nullptr) editor->removeComponentListener(this); }
    void componentMovedOrResized(juce::Component&, bool, bool) override { updateSize(); }
    void updateSize() {
        if (editor == nullptr) return;
        int edW = editor->getWidth();
        int edH = editor->getHeight();
        if (edW <= 0 || edH <= 0) return;
        setSize(edW, edH + 30);
        if (auto* dw = dynamic_cast<juce::DocumentWindow*>(getParentComponent()))
            dw->setContentComponentSize(edW, edH + 30);
    }
    void resized() override {
        header.setBounds(0, 0, getWidth(), 30);
        if (editor != nullptr) editor->setBounds(0, 30, getWidth(), getHeight() - 30);
    }
private:
    VSTCustomHeader header;
    std::unique_ptr<juce::AudioProcessorEditor> editor;
};

class VSTWindow : public juce::DocumentWindow {
public:
    VSTWindow(juce::AudioProcessor* plugin, BaseEffect* fx, TrackContainer* container, std::function<void()> onUpdate)
        : DocumentWindow(plugin->getName(), juce::Colours::darkgrey, DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(false);
        setTitleBarHeight(0);
        setAlwaysOnTop(true);
        if (auto* editor = plugin->createEditorIfNeeded()) {
            setContentOwned(new VSTContainer(this, editor, fx, container, onUpdate), true);
            setResizable(editor->isResizable(), false);
        }
    }
    void closeButtonPressed() override { setVisible(false); }
};

class VSTHost : public BaseEffect, private juce::AudioProcessorParameter::Listener {
public:
    VSTHost();
    ~VSTHost();

    void parameterValueChanged(int parameterIndex, float newValue) override {}
    void parameterGestureChanged(int parameterIndex, bool starting) override {
        if (starting) {
            if (vstPlugin != nullptr && parameterIndex >= 0 && parameterIndex < vstPlugin->getParameters().size()) {
                if (auto* p = vstPlugin->getParameters()[parameterIndex]) {
                    baseValues.set(parameterIndex, p->getValue());
                }
            }

            if (GridModulator::pendingModulator != nullptr) {
                auto* m = GridModulator::pendingModulator;
                ModTarget t;
                t.type = ModTarget::PluginParam;
                t.parameterIdx = parameterIndex;
                t.pluginIdx = myPluginIdx;
                m->addTarget(t);
            }
        }
    }

    int myPluginIdx = -1;
    void loadPluginAsync(double sampleRate, std::function<void(bool)> callback);
    void loadPluginFromPath(const juce::String& path, double sampleRate, std::function<void(bool)> callback);

    bool isLoaded() const override;
    void showWindow(TrackContainer* container = nullptr) override;
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, const juce::AudioBuffer<float>* sidechainBuffer) override;
    
    bool supportsSidechain() const override;
    void refreshSidechainSupport();
    void reset() override;
    void setNonRealtime(bool isNonRealtime) override;
    juce::String getLoadedPluginName() const override;
    juce::String getPluginPath() const { return pluginPath; }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    void updatePlayHead(bool isPlaying, int64_t samplePos) override;
    int getLatencySamples() const override;

    bool isBypassed() const override { return bypassed; }
    void setBypassed(bool shouldBypass) override { bypassed = shouldBypass; }

    // --- ACCUMULATIVE MODULATION ENGINE ---
    void setParameterModulation(int index, float delta) override {
        if (index >= 0 && index < 2048) {
            accumulatedModulation[index] += delta;
        }
    }

    void applyModulations() override {
        if (vstPlugin == nullptr) return;
        auto& params = vstPlugin->getParameters();
        int count = (int)params.size();

        for (int i = 0; i < juce::jmin(count, 2048); ++i) {
            float totalDelta = accumulatedModulation[i];
            
            // Umbral de eficiencia: Si no hay cambio respecto al último bloque, saltamos.
            if (std::abs(totalDelta) < 0.00001f && std::abs(lastAppliedModulation[i]) < 0.00001f) {
                 accumulatedModulation[i] = 0.0f;
                 continue; 
            }

            if (auto* p = params[i]) {
                if (!baseValues.contains(i))
                    baseValues.set(i, p->getValue());

                float base = baseValues[i];
                float newVal = juce::jlimit(0.0f, 1.0f, base + totalDelta);
                
                if (std::abs(p->getValue() - newVal) > 0.0001f) {
                    p->setValueNotifyingHost(newVal);
                }
            }
            lastAppliedModulation[i] = totalDelta;
            accumulatedModulation[i] = 0.0f; // Reset para el siguiente bloque
        }
    }

    std::function<void()> onTopologyUpdate;

private:
    std::atomic<bool> isInitializing { false };
    std::atomic<bool> sidechainSupported { false };
    
    juce::AudioPluginFormatManager formatManager;
    std::unique_ptr<juce::AudioPluginInstance> vstPlugin;
    std::unique_ptr<VSTWindow> vstWindow;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::String pluginPath;
    bool bypassed = false;

    DawPlayHead playHead;
    juce::AudioBuffer<float> fallbackBuffer;
    juce::HashMap<int, float> baseValues; 
    float accumulatedModulation[2048]{ 0.0f };
    float lastAppliedModulation[2048]{ 0.0f };
};