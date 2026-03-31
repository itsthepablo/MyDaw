#pragma once
#include <JuceHeader.h>
#include <functional>
#include "../Native_Plugins/BaseEffect.h"

// ==============================================================================
// RELOJ MAESTRO PARA EL VST3 (Requerido para EQs Lineales y LFO Tools)
// ==============================================================================
class DawPlayHead : public juce::AudioPlayHead {
public:
    juce::Optional<PositionInfo> getPosition() const override {
        juce::AudioPlayHead::PositionInfo info;
        info.setIsPlaying(isPlaying);
        info.setTimeInSamples(currentSample);
        info.setBpm(120.0); // Bpm genérico para mantener la estabilidad
        return info;
    }

    bool isPlaying = false;
    int64_t currentSample = 0;
};

class VSTCustomHeader : public juce::Component {
public:
    VSTCustomHeader(juce::DocumentWindow* w) : window(w) {
        addAndMakeVisible(closeBtn);
        closeBtn.setButtonText("x");
        closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(150, 40, 40));
        closeBtn.onClick = [this] { window->closeButtonPressed(); };
    }
    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(20, 22, 25));
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(juce::Font(15.0f, juce::Font::bold));
        g.drawText(window->getName(), getLocalBounds(), juce::Justification::centred);
    }
    void resized() override {
        int bs = getHeight() - 8;
        closeBtn.setBounds(getWidth() - bs - 8, 4, bs, bs);
    }
    void mouseDown(const juce::MouseEvent& e) override { dragger.startDraggingComponent(window, e); }
    void mouseDrag(const juce::MouseEvent& e) override { dragger.dragComponent(window, e, nullptr); }
private:
    juce::DocumentWindow* window;
    juce::TextButton closeBtn;
    juce::ComponentDragger dragger;
};

class VSTContainer : public juce::Component, private juce::ComponentListener {
public:
    VSTContainer(juce::DocumentWindow* w, juce::AudioProcessorEditor* ed) : header(w), editor(ed) {
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
    VSTWindow(juce::AudioProcessor* plugin)
        : DocumentWindow(plugin->getName(), juce::Colours::darkgrey, DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar(false);
        setTitleBarHeight(0);
        setAlwaysOnTop(true);
        
        if (auto* editor = plugin->createEditorIfNeeded()) {
            setContentOwned(new VSTContainer(this, editor), true);
            setResizable(editor->isResizable(), false);
        }
    }
    void closeButtonPressed() override { setVisible(false); }
};
class VSTHost : public BaseEffect {
public:
    VSTHost();
    ~VSTHost();
    void loadPluginAsync(double sampleRate, std::function<void(bool)> callback);
    void loadPluginFromPath(const juce::String& path, double sampleRate, std::function<void(bool)> callback);

    bool isLoaded() const override;
    void showWindow() override;
    void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages, const juce::AudioBuffer<float>* sidechainBuffer) override;
    
    bool supportsSidechain() const override;
    void reset() override;
    void setNonRealtime(bool isNonRealtime) override;
    juce::String getLoadedPluginName() const override;
    juce::String getPluginPath() const { return pluginPath; }

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Conexión del reloj
    void updatePlayHead(bool isPlaying, int64_t samplePos) override;

    int getLatencySamples() const override;

    bool isBypassed() const override { return bypassed; }
    void setBypassed(bool shouldBypass) override { bypassed = shouldBypass; }

private:
    juce::AudioPluginFormatManager formatManager;
    std::unique_ptr<juce::AudioPluginInstance> vstPlugin;
    std::unique_ptr<VSTWindow> vstWindow;
    std::unique_ptr<juce::FileChooser> fileChooser;
    juce::String pluginPath;
    bool bypassed = false;

    DawPlayHead playHead; // El reloj interno del host
    juce::AudioBuffer<float> fallbackBuffer; // Buffer pre-asignado para evitar locks de memoria
};