#include "MixerComponent.h"

// ==========================================================
// CANAL INDIVIDUAL DEL MIXER (MixerStrip)
// ==========================================================
MixerStrip::MixerStrip(Track& t) : track(t) {
    lastDepth = track.folderDepth;
    lastVis = track.isShowingInChildren;
    lastColor = track.getColor();

    addAndMakeVisible(volSlider);
    volSlider.setSliderStyle(juce::Slider::LinearVertical);
    volSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 15);
    volSlider.setRange(0.0, 1.0);
    volSlider.setValue(track.getVolume(), juce::dontSendNotification);
    volSlider.setLookAndFeel(&flLookAndFeel);
    volSlider.onValueChange = [this] { track.setVolume((float)volSlider.getValue()); };

    addAndMakeVisible(panSlider);
    panSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    panSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    panSlider.setRange(-1.0, 1.0);
    panSlider.setValue(track.getBalance(), juce::dontSendNotification);
    panSlider.setLookAndFeel(&flLookAndFeel);
    panSlider.onValueChange = [this] { track.setBalance((float)panSlider.getValue()); };

    addAndMakeVisible(nameLabel);
    nameLabel.setText(track.getName(), juce::dontSendNotification);
    nameLabel.setJustificationType(juce::Justification::centred);
    nameLabel.setFont(juce::Font(13.0f, juce::Font::bold));

    addAndMakeVisible(levelMeter);
}

void MixerStrip::syncWithTrack() {
    if (nameLabel.getText() != track.getName())
        nameLabel.setText(track.getName(), juce::dontSendNotification);

    if (volSlider.getValue() != track.getVolume())
        volSlider.setValue(track.getVolume(), juce::dontSendNotification);

    if (panSlider.getValue() != track.getBalance())
        panSlider.setValue(track.getBalance(), juce::dontSendNotification);

    // --- MATEMÁTICA POST-FADER Y POST-PAN ---
    float vol = track.getVolume();
    float pan = track.getBalance();
    float leftGain = vol * (pan < 0.0f ? 1.0f : 1.0f - pan);
    float rightGain = vol * (pan > 0.0f ? 1.0f : 1.0f + pan);

    levelMeter.setLevel(track.currentPeakLevelL * leftGain, track.currentPeakLevelR * rightGain);

    if (lastDepth != track.folderDepth || lastColor != track.getColor()) {
        lastDepth = track.folderDepth;
        lastColor = track.getColor();
        repaint();
    }
}

void MixerStrip::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(35, 38, 42));

    if (track.folderDepth > 0) {
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.fillRect(getLocalBounds());
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawVerticalLine(0, 0, (float)getHeight());
    }

    g.setColour(track.getColor());
    g.fillRect(0, getHeight() - 5, getWidth(), 5);

    g.setColour(juce::Colour(20, 22, 25));
    g.drawRect(getLocalBounds(), 1);
}

void MixerStrip::resized() {
    auto area = getLocalBounds().reduced(2);

    nameLabel.setBounds(area.removeFromBottom(25));
    area.removeFromBottom(5);

    panSlider.setBounds(area.removeFromTop(45).reduced(5));
    area.removeFromTop(5);

    auto meterArea = area.removeFromRight(14);
    levelMeter.setBounds(meterArea.reduced(0, 2));

    area.removeFromRight(5);
    volSlider.setBounds(area);
}

// ==========================================================
// CONSOLA PRINCIPAL (MixerComponent)
// ==========================================================
MixerComponent::MixerComponent() {
    addAndMakeVisible(masterVol);
    masterVol.setSliderStyle(juce::Slider::LinearVertical);
    masterVol.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 20);
    masterVol.setRange(0.0, 1.0);
    masterVol.setValue(0.8, juce::dontSendNotification);
    masterVol.setLookAndFeel(&flLookAndFeel);

    addAndMakeVisible(masterLabel);
    masterLabel.setText("MASTER", juce::dontSendNotification);
    masterLabel.setJustificationType(juce::Justification::centred);
    masterLabel.setFont(juce::Font(14.0f, juce::Font::bold));

    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&stripContainer, false);
    viewport.setScrollBarsShown(false, true);

    startTimerHz(30);
}

MixerComponent::~MixerComponent() {
    stopTimer();
}

void MixerComponent::setTracksReference(const juce::OwnedArray<Track>* tracks) {
    tracksRef = tracks;
    updateStrips();
}

float MixerComponent::getMasterVolume() const {
    return (float)masterVol.getValue();
}

void MixerComponent::updateStrips() {
    strips.clear();
    if (tracksRef) {
        for (auto* t : *tracksRef) {
            auto* strip = new MixerStrip(*t);
            strips.add(strip);
            stripContainer.addAndMakeVisible(strip);
        }
    }
    resized();
}

void MixerComponent::timerCallback() {
    if (!tracksRef) return;

    bool needsRebuild = (tracksRef->size() != strips.size());
    if (!needsRebuild) {
        for (int i = 0; i < tracksRef->size(); ++i) {
            if (&(strips[i]->track) != (*tracksRef)[i]) {
                needsRebuild = true;
                break;
            }
        }
    }

    if (needsRebuild) {
        updateStrips();
    }
    else {
        bool needsLayout = false;
        for (int i = 0; i < strips.size(); ++i) {
            auto* t = (*tracksRef)[i];

            if (strips[i]->lastVis != t->isShowingInChildren || strips[i]->lastDepth != t->folderDepth) {
                strips[i]->lastVis = t->isShowingInChildren;
                strips[i]->lastDepth = t->folderDepth;
                needsLayout = true;
            }
            strips[i]->syncWithTrack();
        }

        if (needsLayout) resized();
    }
}

void MixerComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(20, 22, 25));
    g.setColour(juce::Colours::black);
    g.drawVerticalLine(90, 0, (float)getHeight());
}

void MixerComponent::resized() {
    if (tracksRef && tracksRef->size() != strips.size()) return;

    int masterW = 90;
    auto masterArea = getLocalBounds().removeFromLeft(masterW);
    masterLabel.setBounds(masterArea.removeFromBottom(30));
    masterArea.removeFromBottom(5);
    masterVol.setBounds(masterArea.reduced(5));

    viewport.setBounds(getLocalBounds().withTrimmedLeft(masterW));

    int x = 0;
    int stripW = 80;

    int totalExpectedWidth = 0;
    if (tracksRef) {
        for (auto* t : *tracksRef) {
            if (t->isShowingInChildren) totalExpectedWidth += stripW;
        }
    }

    int containerH = viewport.getHeight();
    if (totalExpectedWidth > viewport.getWidth()) {
        containerH -= viewport.getScrollBarThickness();
    }

    containerH = juce::jmax(0, containerH);

    if (tracksRef) {
        for (int i = 0; i < tracksRef->size(); ++i) {
            if (i >= strips.size()) break;

            auto* t = (*tracksRef)[i];

            if (!t->isShowingInChildren) {
                strips[i]->setVisible(false);
                continue;
            }

            strips[i]->setVisible(true);

            const int offsetPerLevel = 8;
            const int maxOffset = juce::jmax(0, containerH / 2);

            int bottomOffset = juce::jlimit(0, maxOffset, t->folderDepth * offsetPerLevel);
            int xIndent = (t->folderDepth > 0) ? 2 : 0;

            strips[i]->setBounds(x + xIndent, 0, stripW - xIndent, containerH - bottomOffset);
            x += stripW;
        }
    }

    stripContainer.setBounds(0, 0, x, containerH);
}