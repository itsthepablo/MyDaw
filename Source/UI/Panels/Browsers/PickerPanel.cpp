#include "PickerPanel.h"
#include "../../../Theme/CustomTheme.h"
#include "../../../Tracks/UI/TrackContainer.h"
#include "../../../Clips/Audio/AudioClip.h"
#include "../../../Clips/Midi/MidiPattern.h"
#include "../../../Clips/Audio/UI/AudioClipRenderer.h"
#include "../../../Clips/Midi/UI/MidiPatternRenderer.h"

PickerPanel::PickerPanel() {
    addAndMakeVisible(headerLabel);
    headerLabel.setText("PICKER", juce::dontSendNotification);
    headerLabel.setJustificationType(juce::Justification::centred);
    headerLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));
    headerLabel.setFont(juce::Font(12.0f, juce::Font::bold));

    addAndMakeVisible(listBox);
    listBox.setModel(this);
    
    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel()))
        listBox.setColour(juce::ListBox::backgroundColourId, theme->getSkinColor("PICKER_BG", juce::Colour(20, 22, 25)));
    else
        listBox.setColour(juce::ListBox::backgroundColourId, juce::Colour(20, 22, 25));

    listBox.setRowHeight(45);

    startTimerHz(2);
}

void PickerPanel::setTrackContainer(TrackContainer* tc) {
    trackContainer = tc;
    refreshList();
}

void PickerPanel::refreshList() {
    if (!trackContainer) return;
    std::vector<PickerItem> newItems;

    for (auto* t : trackContainer->getTracks()) {
        for (auto* a : t->getAudioClips()) newItems.push_back({ a->getName(), t->getColor(), 0, a, nullptr, nullptr, 1 });
        for (auto* m : t->getMidiClips()) newItems.push_back({ m->getName(), t->getColor(), 1, nullptr, m, nullptr, 1 });
    }

    for (auto* a : trackContainer->unusedAudioPool) {
        newItems.push_back({ a->getName(), juce::Colours::darkgrey, 0, a, nullptr, nullptr, 0 });
    }
    for (auto* m : trackContainer->unusedMidiPool) {
        newItems.push_back({ m->getName(), m->getColor(), 1, nullptr, m, nullptr, 0 });
    }
    
    for (auto* aut : trackContainer->automationPool) {
        newItems.push_back({ aut->name, aut->color, 2, nullptr, nullptr, aut, aut->isShowing ? 1 : 0 });
    }

    std::vector<PickerItem> uniqueItems;
    for (const auto& item : newItems) {
        bool found = false;
        for (auto& u : uniqueItems) {
            if (u.name == item.name) {
                u.useCount += item.useCount;
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

void PickerPanel::timerCallback() { refreshList(); }

int PickerPanel::getNumRows() { return (int)items.size(); }

juce::var PickerPanel::getDragSourceDescription(const juce::SparseSet<int>& selectedRows) {
    if (selectedRows.isEmpty()) return {};
    int row = selectedRows[0];
    if (row < 0 || row >= (int)items.size()) return {};

    auto& item = items[row];
    juce::String typeStr = item.type == 1 ? "MIDI" : (item.type == 2 ? "AUTOMATION" : "AUDIO");
    return "PickerDrag|" + typeStr + "|" + item.name;
}

void PickerPanel::listBoxItemClicked(int row, const juce::MouseEvent& e) {
    if (row < 0 || row >= (int)items.size()) return;
    auto item = items[row];
            
    if (e.mods.isRightButtonDown()) {
        listBox.selectRow(row);
        juce::PopupMenu m;
        m.addItem(1, "Eliminar permanentemente");
        m.showMenuAsync(juce::PopupMenu::Options(), [this, item](int res) {
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

void PickerPanel::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) {
    if (rowNumber < 0 || rowNumber >= (int)items.size()) return;
    auto& item = items[rowNumber];

    if (rowIsSelected) g.fillAll(juce::Colours::white.withAlpha(0.1f));

    juce::Rectangle<int> clipRect(10, 5, width - 35, height - 10);

    g.setColour(item.color.withAlpha(0.2f));
    g.fillRoundedRectangle(clipRect.toFloat(), 4.0f);

    if (item.type == 1 && item.midiRef != nullptr) {
        float miniZoom = (float)clipRect.getWidth() / std::max(10.0f, item.midiRef->getWidth());
        float startOffset = item.midiRef->getStartX() * miniZoom;
        MidiPatternRenderer::drawMidiPattern(g, *item.midiRef, clipRect, item.color, item.name, false, miniZoom, (int)startOffset, 0.0);
    }
    else if (item.type == 0 && item.audioRef != nullptr) {
        double miniZoom = (double)clipRect.getWidth() / std::max(0.1, (double)item.audioRef->getWidth());
        AudioClipRenderer::drawAudioClip(g, *item.audioRef, clipRect.toFloat(), item.color, WaveformViewMode::Combined, miniZoom, 0.0, 0.0);
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
}

void PickerPanel::lookAndFeelChanged() {
    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel()))
        listBox.setColour(juce::ListBox::backgroundColourId, theme->getSkinColor("PICKER_BG", juce::Colour(20, 22, 25)));
    repaint();
}

void PickerPanel::paint(juce::Graphics& g) {
    if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel()))
        g.fillAll(theme->getSkinColor("PICKER_BG", juce::Colour(20, 22, 25)));
    else
        g.fillAll(juce::Colour(20, 22, 25));

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawVerticalLine(getWidth() - 1, 0, (float)getHeight());
}

void PickerPanel::resized() {
    auto area = getLocalBounds();
    headerLabel.setBounds(area.removeFromTop(30));
    listBox.setBounds(area);
}
