#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h"
#include "../Effects/EffectsPanel.h" 
#include "../Effects/EffectDevice.h"

class InstrumentPanel : public juce::Component {
public:
    InstrumentPanel();
    ~InstrumentPanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override { repaint(); }

    void setTrack(Track* t);
    void updateInstrumentView();

    // Callbacks que enviamos al Bridge
    std::function<void(Track&)> onAddInstrument;
    std::function<void(Track&)> onAddNativeInstrument;
    std::function<void(Track&, BaseEffect*)> onOpenInstrumentWindow;

private:
    Track* activeTrack = nullptr;
    juce::TextButton addInstrumentBtn;

    // Matriz dinámica para soportar múltiples slots de instrumentos/efectos
    juce::OwnedArray<EffectDevice> devices;
    juce::Array<juce::Component*> nativeEditors;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InstrumentPanel)
};