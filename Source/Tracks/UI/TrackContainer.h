#pragma once
#include <JuceHeader.h>
#include <functional>
#include "../../Theme/CustomTheme.h"
#include "../../Data/Track.h"
#include "TrackControlPanel.h"
#include "MasterTrackStrip.h"
#include <vector>
#include <algorithm>
#include <map>

/**
 * TrackHeaderBackground — Dibujado del fondo de la cabecera de pistas.
 */
struct TrackHeaderBackground : public juce::Component {
    TrackHeaderBackground() { setOpaque(true); }
    void paint(juce::Graphics& g) override {
        if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
            g.fillAll(theme->getSkinColor("TRACKS_BG", juce::Colour(25, 27, 30)));
        } else {
            g.fillAll(juce::Colour(25, 27, 30));
        }
    }
};

/**
 * TrackContainer — Contenedor principal de los paneles de pista.
 * Gestiona el listado de pistas, jerarquía de carpetas y drag-and-drop.
 */
class TrackContainer : public juce::Component,
    public juce::DragAndDropTarget,
    public juce::FileDragAndDropTarget {
public:
    std::function<void(Track&, TrackControlPanel&)> onLoadFx;
    std::function<void(Track&)> onOpenInstrument; 
    std::function<void(Track&, int)> onOpenFx;
    std::function<void(Track&)> onOpenPianoRoll;
    std::function<void(int)> onDeleteTrack;
    std::function<void(Track&)> onOpenEffects;
    std::function<void()> onTracksReordered;
    std::function<void()> onTrackAdded;
    std::function<void(Track*)> onActiveTrackChanged;
    std::function<void(float)> onScrollWheel;
    std::function<void(TrackType, bool)> onToggleAnalysisTrack;
    std::function<void()> onDeselectMasterRequested;
    
    int vOffset = 0;
    float trackHeight = 100.0f;

    struct TrackContent : public juce::Component {
        TrackContent() { setInterceptsMouseClicks(false, true); }
    } trackContent;

    juce::OwnedArray<AudioClip> unusedAudioPool;
    juce::OwnedArray<MidiPattern> unusedMidiPool;
    juce::OwnedArray<AutomationClipData> automationPool;

    TrackContainer();
    ~TrackContainer() override = default;

    void setExternalMutex(juce::CriticalSection* mutex) { audioMutex = mutex; }
    const juce::OwnedArray<Track>& getTracks() const { return tracks; }

    void setTrackHeight(float newHeight);
    void deselectAllTracks();
    void selectTrack(Track* selectedTrack, const juce::ModifierKeys& mods);
    Track* addTrack(TrackType type, int idToUse = -1);
    Track* getMasterTrack();
    void removeTrack(int index);
    void deleteUnusedClipsByName(const juce::String& name, bool isMidi);
    
    AutomationClipData* createAutomation(int targetTrackId, int parameterId, const juce::String& paramName);

    void setVOffset(int newOffset) { vOffset = newOffset; resized(); }
    void recalculateFolderHierarchy();
    void moveSelectedTracksToNewFolder();
    void refreshTrackPanels();
    void setAnalysisTrackToggleState(TrackType type, bool state);

    // Drag-and-drop
    bool isInterestedInDragSource(const SourceDetails& d) override { return d.description == "TRACK"; }
    void itemDragMove(const SourceDetails& d) override;
    void itemDragExit(const SourceDetails& d) override { for (auto* p : trackPanels) { p->dragHoverMode = 0; p->repaint(); } repaint(); }
    void itemDropped(const SourceDetails& d) override;

    bool isInterestedInFileDrag(const juce::StringArray&) override { return true; }
    void filesDropped(const juce::StringArray& files, int, int) override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void updateStyles();
    void lookAndFeelChanged() override { updateStyles(); headerBg.repaint(); repaint(); }
    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override;

private:
    TrackHeaderBackground headerBg;
    juce::TextButton addMidiBtn{ "+ MIDI" }, addAudioBtn{ "+ AUDIO" };
    juce::TextButton toggleLdnBtn{ "LDN" }, toggleBalBtn{ "BAL" }, toggleMsBtn{ "MS" };
    juce::OwnedArray<Track> tracks;
    juce::OwnedArray<TrackControlPanel> trackPanels;
    juce::CriticalSection* audioMutex = nullptr;
    int lastSelectedTrackIndex = -1;
    int nextTrackId = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TrackContainer)
};