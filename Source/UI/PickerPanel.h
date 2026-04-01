#pragma once
#include <JuceHeader.h>
#include "../Tracks/TrackContainer.h"
#include "MidiClipRenderer.h"   
#include "WaveformRenderer.h"   
#include <vector>
#include <algorithm>

struct PickerItem {
    juce::String name;
    juce::Colour color;
    int type; // 0 = Audio, 1 = Midi, 2 = Automation
    AudioClipData* audioRef = nullptr;
    MidiClipData* midiRef = nullptr;
    AutomationClipData* autoRef = nullptr;
    int useCount = 0;

    bool operator==(const PickerItem& other) const {
        return name == other.name && color == other.color && type == other.type && useCount == other.useCount;
    }
};

class PickerPanel : public juce::Component, public juce::ListBoxModel, private juce::Timer {
public:
    std::function<void(juce::String, int)> onDeleteRequested;
    std::function<void(AutomationClipData*)> onAutomationSelected;

    PickerPanel() {
        addAndMakeVisible(headerLabel);
        headerLabel.setText("PICKER", juce::dontSendNotification);
        headerLabel.setJustificationType(juce::Justification::centred);
        headerLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));
        headerLabel.setFont(juce::Font(12.0f, juce::Font::bold));

        addAndMakeVisible(listBox);
        listBox.setModel(this);
        listBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(20, 22, 25));

        listBox.setRowHeight(45);

        startTimerHz(2);
    }

    void setTrackContainer(TrackContainer* tc) {
        trackContainer = tc;
        refreshList();
    }

    void refreshList() {
        if (!trackContainer) return;
        std::vector<PickerItem> newItems;

        // 1. Escaneamos los tracks activos (suman 1 al useCount)
        for (auto* t : trackContainer->getTracks()) {
            for (auto* a : t->audioClips) newItems.push_back({ a->name, t->getColor(), 0, a, nullptr, nullptr, 1 });
            for (auto* m : t->midiClips) newItems.push_back({ m->name, t->getColor(), 1, nullptr, m, nullptr, 1 });
        }

        // 2. Escaneamos el pool de inactivos (suman 0 al useCount)
        for (auto* a : trackContainer->unusedAudioPool) {
            newItems.push_back({ a->name, juce::Colours::darkgrey, 0, a, nullptr, nullptr, 0 });
        }
        for (auto* m : trackContainer->unusedMidiPool) {
            newItems.push_back({ m->name, m->color, 1, nullptr, m, nullptr, 0 });
        }
        
        // 3. Escaneamos automaciones 
        for (auto* aut : trackContainer->automationPool) {
            newItems.push_back({ aut->name, aut->color, 2, nullptr, nullptr, aut, aut->isShowing ? 1 : 0 });
        }

        std::vector<PickerItem> uniqueItems;
        for (const auto& item : newItems) {
            bool found = false;
            for (auto& u : uniqueItems) {
                if (u.name == item.name) {
                    u.useCount += item.useCount; // Sumamos usos
                    found = true; break;
                }
            }
            if (!found) {
                uniqueItems.push_back(item);
            }
        }

        bool changed = items.size() != uniqueItems.size();
        if (!changed) {
            for (size_t i = 0; i < items.size(); ++i) {
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

    juce::var getDragSourceDescription(const juce::SparseSet<int>& selectedRows) override {
        if (selectedRows.isEmpty()) return {};
        int row = selectedRows[0];
        if (row < 0 || row >= items.size()) return {};

        auto& item = items[row];
        juce::String typeStr = item.type == 1 ? "MIDI" : (item.type == 2 ? "AUTOMATION" : "AUDIO");
        return "PickerDrag|" + typeStr + "|" + item.name;
    }

    // --- NUEVO: Clic Derecho para borrado permanente ---
    void listBoxItemClicked(int row, const juce::MouseEvent& e) override {
        if (row < 0 || row >= items.size()) return;
        auto item = items[row];
                
        if (e.mods.isRightButtonDown()) {
            listBox.selectRow(row);
            juce::PopupMenu m;
            m.addItem(1, "Eliminar permanentemente");
            m.showMenuAsync(juce::PopupMenu::Options(), [this, row, item](int res) {
                if (res == 1) {
                    if (onDeleteRequested) onDeleteRequested(item.name, item.type);
                }
            });
        }
        else if (e.mods.isLeftButtonDown()) {
            if (item.type == 2 && item.autoRef && trackContainer) {
                bool wasShowing = item.autoRef->isShowing;
                for (auto* aut : trackContainer->automationPool) {
                    if (aut && aut->targetTrackId == item.autoRef->targetTrackId) {
                        aut->isShowing = false;
                    }
                }
                item.autoRef->isShowing = !wasShowing;
                trackContainer->repaint();
                if (auto* top = getTopLevelComponent()) top->repaint();
                
                if (onAutomationSelected) onAutomationSelected(item.autoRef);
                refreshList();
            }
        }
    }

    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override {
        if (rowNumber < 0 || rowNumber >= items.size()) return;
        auto& item = items[rowNumber];

        if (rowIsSelected) g.fillAll(juce::Colours::white.withAlpha(0.1f));

        juce::Rectangle<int> clipRect(10, 5, width - 35, height - 10);

        g.setColour(item.color.withAlpha(0.2f));
        g.fillRoundedRectangle(clipRect.toFloat(), 4.0f);

        if (item.type == 1 && item.midiRef != nullptr) {
            float miniZoom = (float)clipRect.getWidth() / std::max(10.0f, item.midiRef->width);
            float startOffset = item.midiRef->startX * miniZoom;
            MidiClipRenderer::drawMidiClip(g, *item.midiRef, clipRect, item.color, false, miniZoom, startOffset);
        }
        else if (item.type == 0 && item.audioRef != nullptr) {
            WaveformRenderer::drawWaveform(g, *item.audioRef, clipRect, item.color, WaveformViewMode::Combined);
        }
        else if (item.type == 2 && item.autoRef != nullptr) {
            g.setColour(item.color.brighter());
            float cy = clipRect.getCentreY();
            g.drawHorizontalLine((int)cy, (float)clipRect.getX() + 5.0f, (float)clipRect.getRight() - 5.0f);
            g.fillEllipse((float)clipRect.getX() + 5.0f, cy - 3.0f, 6.0f, 6.0f);
            g.fillEllipse((float)clipRect.getRight() - 11.0f, cy - 3.0f, 6.0f, 6.0f);
        }

        g.setColour(item.color.withAlpha(0.6f));
        g.drawRoundedRectangle(clipRect.toFloat(), 4.0f, 1.0f);

        juce::Rectangle<int> textRect(18, 5, width - 40, height - 10);
        g.setFont(juce::Font(13.0f, juce::Font::bold));

        g.setColour(juce::Colours::black.withAlpha(0.9f));
        g.drawText(item.name, textRect.translated(1, 1), juce::Justification::centredLeft, true);
        g.drawText(item.name, textRect.translated(-1, -1), juce::Justification::centredLeft, true);
        g.drawText(item.name, textRect.translated(1, -1), juce::Justification::centredLeft, true);
        g.drawText(item.name, textRect.translated(-1, 1), juce::Justification::centredLeft, true);

        g.setColour(juce::Colours::white);
        g.drawText(item.name, textRect, juce::Justification::centredLeft, true);

        juce::Rectangle<float> dotRect((float)width - 18.0f, (float)height / 2.0f - 4.0f, 8.0f, 8.0f);
        if (item.useCount > 0) {
            g.setColour(juce::Colours::lawngreen);
            g.fillEllipse(dotRect);
            g.setColour(juce::Colours::lawngreen.withAlpha(0.3f));
            g.fillEllipse(dotRect.expanded(3.0f));
        }
        else {
            g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
            g.fillEllipse(dotRect);
        }
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