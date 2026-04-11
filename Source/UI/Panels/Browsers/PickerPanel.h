#pragma once
#include <JuceHeader.h>
#include <vector>
#include <functional>

// Forward declarations
class TrackContainer;
class AudioClip;
class MidiPattern;
struct AutomationClipData;

/**
 * PickerItem: Representa un elemento en el panel de seleccion (Picker).
 */
struct PickerItem {
    juce::String name;
    juce::Colour color;
    int type; // 0 = Audio, 1 = Midi, 2 = Automation
    AudioClip* audioRef = nullptr;
    MidiPattern* midiRef = nullptr;
    AutomationClipData* autoRef = nullptr;
    int useCount = 0;

    bool operator==(const PickerItem& other) const {
        return name == other.name && color == other.color && type == other.type && useCount == other.useCount;
    }
};

/**
 * PickerPanel: Panel lateral que gestiona el pool de clips y automatizaciones.
 * Permite arrastrar elementos a la playlist o eliminarlos permanentemente.
 */
class PickerPanel : public juce::Component, 
                    public juce::ListBoxModel, 
                    private juce::Timer 
{
public:
    std::function<void(juce::String, int)> onDeleteRequested;
    std::function<void(AutomationClipData*)> onAutomationSelected;

    PickerPanel();
    ~PickerPanel() override = default;

    void setTrackContainer(TrackContainer* tc);
    void refreshList();

    // ListBoxModel implementation
    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override;
    juce::var getDragSourceDescription(const juce::SparseSet<int>& selectedRows) override;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

    // Timer override
    void timerCallback() override;

private:
    juce::Label headerLabel;
    juce::ListBox listBox;
    TrackContainer* trackContainer = nullptr;
    std::vector<PickerItem> items;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PickerPanel)
};