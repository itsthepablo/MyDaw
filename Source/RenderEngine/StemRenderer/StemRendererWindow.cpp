#include "StemRendererWindow.h"

// --- NUEVA IMPLEMENTACIÓN DE PESTAÑAS (STEMS / MULTITRACK) ---

StemRendererWindow::TrackListTab::TrackListTab(const std::vector<TrackStemInfo>& tracks, bool onlyFolders) {
    for (const auto& t : tracks) {
        bool isFolder = (t.type == TrackType::Folder);
        if (onlyFolders == isFolder) {
            filteredTracks.push_back(t);
        }
    }
    
    addAndMakeVisible(viewport);
    listHolder = new juce::Component();
    viewport.setViewedComponent(listHolder, true);
    viewport.setScrollBarsShown(true, false);
    
    for (int i = 0; i < filteredTracks.size(); ++i) {
        auto* row = new TrackRow(&filteredTracks[i]);
        rows.add(row);
        listHolder->addAndMakeVisible(row);
    }
}

void StemRendererWindow::TrackListTab::getSelectedIds(std::vector<int>& ids) {
    for (const auto& t : filteredTracks) {
        if (t.selected) ids.push_back(t.id);
    }
}

void StemRendererWindow::TrackListTab::resized() {
    viewport.setBounds(getLocalBounds());
    
    int rowHeight = 35;
    int safeWidth = viewport.getMaximumVisibleWidth();
    if (safeWidth <= 0) safeWidth = 400; 

    listHolder->setBounds(0, 0, safeWidth, rows.size() * rowHeight);
    
    for (int i = 0; i < rows.size(); ++i) {
        rows[i]->setBounds(5, i * rowHeight + 2, safeWidth - 10, rowHeight - 4);
    }
}

// -------------------------------------------------------------

StemRendererWindow::TrackRow::TrackRow(TrackStemInfo* i) : info(i) {
    addAndMakeVisible(toggle);
    addAndMakeVisible(nameLabel);
    addAndMakeVisible(typeLabel);
    
    toggle.setToggleState(info->selected, juce::dontSendNotification);
    toggle.onClick = [this] { info->selected = toggle.getToggleState(); };
    toggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::orange);
    
    nameLabel.setText(info->name.isEmpty() ? "Track Sin Nombre" : info->name, juce::dontSendNotification);
    nameLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    
    if (info->type == TrackType::Folder) {
        nameLabel.setFont(juce::Font("Inter", 15.0f, juce::Font::bold));
        typeLabel.setText("[Carpeta]", juce::dontSendNotification);
        typeLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    } else {
        nameLabel.setFont(juce::Font("Inter", 14.0f, juce::Font::plain));
        typeLabel.setText("[Track]", juce::dontSendNotification);
        typeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey.withAlpha(0.5f));
    }
}

void StemRendererWindow::TrackRow::paint(juce::Graphics& g) {
    g.setColour(juce::Colour(75, 78, 82)); 
    g.fillRoundedRectangle(getLocalBounds().reduced(2).toFloat(), 4.0f);
}

void StemRendererWindow::TrackRow::resized() {
    int indent = juce::jlimit(0, 100, info->folderDepth * 15);
    toggle.setBounds(10 + indent, 5, 24, 24);
    nameLabel.setBounds(40 + indent, 5, 200, 24);
    typeLabel.setBounds(getWidth() - 90, 5, 80, 24);
}

// -------------------------------------------------------------

StemRendererWindow::StemRendererWindow(const std::vector<TrackStemInfo>& tracks)
    : stemTracks(tracks)
{
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Exportar Stems / Multitrack", juce::dontSendNotification);
    titleLabel.setFont(juce::Font("Inter", 20.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    
    tabs = std::make_unique<juce::TabbedComponent>(juce::TabbedButtonBar::TabsAtTop);
    addAndMakeVisible(tabs.get());
    
    stemTab = new TrackListTab(stemTracks, true);
    multitrackTab = new TrackListTab(stemTracks, false);
    
    tabs->addTab("Stems (Carpetas)", juce::Colour(45, 47, 50), stemTab, true);
    tabs->addTab("Multitracks (Indiv.)", juce::Colour(45, 47, 50), multitrackTab, true);
    
    tabs->setOutline(0);
    tabs->setIndent(5);
    
    addAndMakeVisible(btnCancel);
    addAndMakeVisible(btnRender);
    
    btnCancel.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 42, 45));
    btnRender.setColour(juce::TextButton::buttonColourId, juce::Colours::darkorange);
    
    btnCancel.onClick = [this] { if (onCancelRequested) onCancelRequested(); };
    btnRender.onClick = [this] {
        std::vector<int> selections;
        auto* activeTab = dynamic_cast<TrackListTab*>(tabs->getCurrentContentComponent());
        if (activeTab) {
            activeTab->getSelectedIds(selections);
        }
        
        if (selections.empty()) {
            juce::NativeMessageBox::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Selección Vacía", "Por favor selecciona al menos un elemento para exportar.");
            return;
        }
        
        if (onStartStemsRequested) onStartStemsRequested(selections);
    };
    
    setSize(500, 600);
}

StemRendererWindow::~StemRendererWindow() {}

void StemRendererWindow::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(30, 32, 35));
}

void StemRendererWindow::resized() {
    auto area = getLocalBounds().reduced(20);
    titleLabel.setBounds(area.removeFromTop(40));
    area.removeFromTop(10);
    
    auto bottomArea = area.removeFromBottom(40);
    btnCancel.setBounds(bottomArea.removeFromLeft(120));
    btnRender.setBounds(bottomArea.removeFromRight(200));
    
    area.removeFromBottom(15);
    tabs->setBounds(area);
}

// -------------------------------------------------------------

StemRendererHost::StemRendererHost(const std::vector<TrackStemInfo>& t)
    : DocumentWindow("Exportar Stems", juce::Colour(30, 32, 35), DocumentWindow::closeButton)
{
    setUsingNativeTitleBar(false);
    setTitleBarHeight(0);
    
    auto* comp = new StemRendererWindow(t);
    comp->onCancelRequested = [this] { closeButtonPressed(); };
    comp->onStartStemsRequested = [this](std::vector<int> stems) {
        if (onExportStarts) onExportStarts(stems);
        closeButtonPressed();
    };
    setContentOwned(comp, true);
}

void StemRendererHost::closeButtonPressed() {
    setVisible(false);
}
