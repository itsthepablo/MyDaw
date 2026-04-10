#pragma once
#include <JuceHeader.h>
#include <vector>
#include <memory>
#include "../../Data/Track.h"

struct TrackStemInfo {
    int id;
    juce::String name;
    int parentId;
    TrackType type;
    int folderDepth;
    bool selected;
};

class StemRendererWindow : public juce::Component {
public:
    StemRendererWindow(const std::vector<TrackStemInfo>& tracks);
    ~StemRendererWindow() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
    std::function<void(std::vector<int>)> onStartStemsRequested;
    std::function<void()> onCancelRequested;

private:
    struct TrackRow : public juce::Component {
        TrackStemInfo* info;
        juce::ToggleButton toggle;
        juce::Label nameLabel;
        juce::Label typeLabel;
        
        TrackRow(TrackStemInfo* i);
        void paint(juce::Graphics& g) override;
        void resized() override;
    };

    // --- NUEVO: Componente para cada Pestaña ---
    struct TrackListTab : public juce::Component {
        TrackListTab(const std::vector<TrackStemInfo>& tracks, bool onlyFolders);
        void resized() override;
        void getSelectedIds(std::vector<int>& ids);
        
        std::vector<TrackStemInfo> filteredTracks;
        juce::OwnedArray<TrackRow> rows;
        juce::Viewport viewport;
        juce::Component* listHolder = nullptr;
    };

    std::vector<TrackStemInfo> stemTracks;
    
    juce::Label titleLabel;
    juce::TextButton btnCancel{"Cancelar"};
    juce::TextButton btnRender{"Exportar Seleccionados"};
    
    std::unique_ptr<juce::TabbedComponent> tabs;
    TrackListTab* stemTab = nullptr;
    TrackListTab* multitrackTab = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StemRendererWindow)
};

class StemRendererHost : public juce::DocumentWindow {
public:
    StemRendererHost(const std::vector<TrackStemInfo>& t);
    void closeButtonPressed() override;
    std::function<void(std::vector<int>)> onExportStarts;
};
