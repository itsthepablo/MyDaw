#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include <vector>

// Estructura ligera para guardar los datos de lo que mostramos en la lista
struct PickerItem {
    juce::String name;
    juce::Colour color;
    bool isMidi;
    
    bool operator==(const PickerItem& other) const {
        return name == other.name && color == other.color && isMidi == other.isMidi;
    }
};

class PickerPanel : public juce::Component, public juce::ListBoxModel, private juce::Timer {
public:
    PickerPanel() {
        addAndMakeVisible(headerLabel);
        headerLabel.setText("PICKER", juce::dontSendNotification);
        headerLabel.setJustificationType(juce::Justification::centred);
        headerLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));
        headerLabel.setFont(juce::Font(12.0f, juce::Font::bold));

        addAndMakeVisible(listBox);
        listBox.setModel(this);
        listBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(20, 22, 25));
        
        // Escanea el proyecto 2 veces por segundo para buscar nuevos patrones
        startTimerHz(2); 
    }

    void setTrackContainer(TrackContainer* tc) {
        trackContainer = tc;
        refreshList();
    }

    void refreshList() {
        if (!trackContainer) return;
        std::vector<PickerItem> newItems;
        
        for (auto* t : trackContainer->getTracks()) {
            for (auto* a : t->audioClips) {
                newItems.push_back({a->name, t->getColor(), false});
            }
            for (auto* m : t->midiClips) {
                newItems.push_back({m->name, t->getColor(), true});
            }
        }

        // Filtramos para no mostrar nombres duplicados (Picker Global)
        std::vector<PickerItem> uniqueItems;
        for(const auto& item : newItems) {
            bool found = false;
            for(const auto& u : uniqueItems) {
                if (u.name == item.name) { found = true; break; }
            }
            if (!found) uniqueItems.push_back(item);
        }

        bool changed = items.size() != uniqueItems.size();
        if (!changed) {
            for(size_t i=0; i<items.size(); ++i) {
                if (!(items[i] == uniqueItems[i])) { changed = true; break; }
            }
        }

        if (changed) {
            items = uniqueItems;
            listBox.updateContent();
            repaint();
        }
    }

    void timerCallback() override { refreshList(); }

    int getNumRows() override { return (int)items.size(); }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override {
        if (rowNumber < 0 || rowNumber >= items.size()) return;
        auto& item = items[rowNumber];
        
        if (rowIsSelected) g.fillAll(juce::Colours::white.withAlpha(0.1f));
        
        // Dibuja un cuadrado para MIDI y un círculo para Audio
        juce::Rectangle<int> iconArea(8, 6, height - 12, height - 12);
        g.setColour(item.color);
        if (item.isMidi) g.fillRect(iconArea);
        else g.fillEllipse(iconArea.toFloat());

        g.setColour(juce::Colours::white.withAlpha(0.9f));
        g.setFont(14.0f);
        g.drawText(item.name, 28, 0, width - 30, height, juce::Justification::centredLeft, true);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(20, 22, 25));
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawVerticalLine(getWidth() - 1, 0, (float)getHeight());
    }

    void resized() override {
        auto area = getLocalBounds();
        headerLabel.setBounds(area.removeFromTop(30));
        listBox.setBounds(area);
    }

private:
    juce::Label headerLabel;
    juce::ListBox listBox;
    TrackContainer* trackContainer = nullptr;
    std::vector<PickerItem> items;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PickerPanel)
};