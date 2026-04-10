#pragma once
#include <JuceHeader.h>
#include <cmath>
#include "TopMenuButton.h"
#include "WindowControlButton.h"
#include "BpmDragControl.h"
#include "RecordButton.h"
#include "SvgToggleButton.h"
#include "../../../Theme/CustomTheme.h"

class TopMenuBar : public juce::Component, public juce::Timer {
public:
    juce::TextButton viewToggleBtn;
    juce::TextButton playlistToggleBtn;
    SvgToggleButton playBtn{ "PLAY", "PAUSE" };
    juce::TextButton stopBtn{ "STOP" };
    BpmDragControl bpmControl;
    juce::TextButton metronomeBtn{ "MET" };
    RecordButton recordBtn;

    juce::TextButton pickerBtn{ "Pick" };
    juce::TextButton filesBtn{ "Files" };
    juce::TextButton mixerBtn{ "Mixer" };
    juce::TextButton rackBtn{ "Rack" };
    juce::TextButton fxBtn{ "Effects" };

    std::function<void()> onSaveRequested;
    std::function<void()> onExportRequested; 
    std::function<void()> onExportStemsRequested;
    std::function<void()> onThemeManagerRequested;
    std::function<double()> requestPlaybackTimeInSeconds;

    std::function<void()> onTogglePicker, onToggleFiles, onToggleMixer, onToggleRack, onToggleFx;
    std::function<void(TrackType)> onNewTrackRequested;

    TopMenuBar() {
        addAndMakeVisible(btnFile);
        addAndMakeVisible(btnEdit);
        addAndMakeVisible(btnCreate);
        addAndMakeVisible(btnView);
        addAndMakeVisible(btnOptions);
        addAndMakeVisible(btnTools);
        addAndMakeVisible(btnHelp);

        btnFile.setTriggeredOnMouseDown(true);
        btnFile.getProperties().set("isMenuBarItem", true);
        btnFile.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btnFile.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);

        btnEdit.setTriggeredOnMouseDown(true);
        btnEdit.getProperties().set("isMenuBarItem", true);
        btnEdit.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btnEdit.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);

        btnCreate.setTriggeredOnMouseDown(true);
        btnCreate.getProperties().set("isMenuBarItem", true);
        btnCreate.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btnCreate.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);

        btnView.setTriggeredOnMouseDown(true);
        btnView.getProperties().set("isMenuBarItem", true);
        btnView.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btnView.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);

        btnOptions.setTriggeredOnMouseDown(true);
        btnOptions.getProperties().set("isMenuBarItem", true);
        btnOptions.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btnOptions.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);

        btnTools.setTriggeredOnMouseDown(true);
        btnTools.getProperties().set("isMenuBarItem", true);
        btnTools.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btnTools.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);

        btnHelp.setTriggeredOnMouseDown(true);
        btnHelp.getProperties().set("isMenuBarItem", true);
        btnHelp.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        btnHelp.setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);

        btnFile.onClick = [this] { showDropdownMenu("FILE", btnFile); };
        btnEdit.onClick = [this] { showDropdownMenu("EDIT", btnEdit); };
        btnCreate.onClick = [this] { showDropdownMenu("CREATE", btnCreate); };
        btnView.onClick = [this] { showDropdownMenu("VIEW", btnView); };
        btnOptions.onClick = [this] { showDropdownMenu("OPTIONS", btnOptions); };
        btnTools.onClick = [this] { showDropdownMenu("TOOLS", btnTools); };
        btnHelp.onClick = [this] { showDropdownMenu("HELP", btnHelp); };

        addAndMakeVisible(viewToggleBtn);
        viewToggleBtn.setButtonText("MIXER / ARRANGEMENT");
        viewToggleBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 45, 50));

        addAndMakeVisible(playlistToggleBtn);
        playlistToggleBtn.setButtonText("PL/CR");
        playlistToggleBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::blue);

        addAndMakeVisible(playBtn);

        addAndMakeVisible(stopBtn);

        addAndMakeVisible(timeDisplayLabel);
        timeDisplayLabel.setText("00:00:00", juce::dontSendNotification);
        timeDisplayLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.5f));
        timeDisplayLabel.setColour(juce::Label::textColourId, juce::Colours::greenyellow);
        timeDisplayLabel.setJustificationType(juce::Justification::centred);
        timeDisplayLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 24.0f, juce::Font::bold));
        timeDisplayLabel.setBorderSize(juce::BorderSize<int>(2));

        addAndMakeVisible(bpmControl);

        addAndMakeVisible(metronomeBtn);
        metronomeBtn.setClickingTogglesState(true);
        metronomeBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::limegreen);
        metronomeBtn.setColour(juce::TextButton::textColourOnId, juce::Colours::black);

        addAndMakeVisible(recordBtn);

        addAndMakeVisible(pickerBtn);
        pickerBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 45, 50));
        pickerBtn.onClick = [this] { if (onTogglePicker) onTogglePicker(); };

        addAndMakeVisible(filesBtn);
        filesBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 45, 50));
        filesBtn.onClick = [this] { if (onToggleFiles) onToggleFiles(); };

        addAndMakeVisible(mixerBtn);
        mixerBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 45, 50));
        mixerBtn.onClick = [this] { if (onToggleMixer) onToggleMixer(); };

        addAndMakeVisible(rackBtn);
        rackBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 45, 50));
        rackBtn.onClick = [this] { if (onToggleRack) onToggleRack(); };

        addAndMakeVisible(fxBtn);
        fxBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 45, 50));
        fxBtn.onClick = [this] { if (onToggleFx) onToggleFx(); };

        addAndMakeVisible(minBtn);
        addAndMakeVisible(maxBtn);
        addAndMakeVisible(closeBtn);

        minBtn.onClick = [this] { if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>()) dw->setMinimised(true); };

        maxBtn.onClick = [this] {
            if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>()) {
                auto userArea = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
                if (dw->getBounds() == userArea) {
                    dw->centreWithSize(1200, 800);
                }
                else {
                    dw->setBounds(userArea);
                }
            }
            };

        closeBtn.onClick = [this] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); };

        startTimerHz(15);
    }

    ~TopMenuBar() override {
        stopTimer();
    }

    void timerCallback() override {
        if (requestPlaybackTimeInSeconds) {
            double timeSecs = requestPlaybackTimeInSeconds();
            timeSecs = std::max(0.0, timeSecs);

            int mins = (int)(timeSecs / 60.0);
            int secs = (int)std::fmod(timeSecs, 60.0);
            int cs = (int)(std::fmod(timeSecs, 1.0) * 100.0);

            juce::String timeStr = juce::String::formatted("%02d:%02d:%02d", mins, secs, cs);

            if (timeDisplayLabel.getText() != timeStr) {
                timeDisplayLabel.setText(timeStr, juce::dontSendNotification);
            }
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        if (auto* w = dynamic_cast<juce::ResizableWindow*>(getTopLevelComponent())) {
            if (w->isFullScreen()) return; // No mover si está maximizado
        }
        dragger.startDraggingComponent(getTopLevelComponent(), e);
    }
    void mouseDrag(const juce::MouseEvent& e) override {
        if (auto* w = dynamic_cast<juce::ResizableWindow*>(getTopLevelComponent())) {
            if (w->isFullScreen()) return;
        }
        dragger.dragComponent(getTopLevelComponent(), e, nullptr);
    }

    void showDropdownMenu(const juce::String& menuName, juce::Button& targetButton) {
        juce::PopupMenu menu;
        menu.setLookAndFeel(&customMenuTheme);

        if (menuName == "FILE") {
            menu.addItem(1, "Nuevo Proyecto", true, false);
            menu.addItem(2, "Abrir Proyecto...", true, false);
            menu.addSeparator();
            menu.addItem(3, "Guardar Proyecto (.perritogordo)", true, false);
            menu.addItem(4, "Guardar como...", true, false);
            // --- INYECCIÓN 2: Opción en el Menú ---
            menu.addSeparator();
            menu.addItem(5, "Exportar Master (WAV)...", true, false);
            menu.addItem(6, "Exportar Stems (Multitrack)...", true, false);
            // --------------------------------------
        }
        else if (menuName == "CREATE") {
            menu.addItem(100, "Inst (MIDI) Track", true, false);
            menu.addItem(101, "Audio Track", true, false);
            menu.addItem(102, "Folder Track", true, false);
        }
        else if (menuName == "VIEW") {
            menu.addItem(1, "Vacio por ahora...", false, false);
        }
        else if (menuName == "TOOLS") {
            menu.addItem(10, "Theme Manager", true, false);
        }
        else {
            menu.addItem(1, "Vacio por ahora...", false, false);
        }

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&targetButton),
            [this](int result) {
                if (result == 100 && onNewTrackRequested) onNewTrackRequested(TrackType::MIDI);
                if (result == 101 && onNewTrackRequested) onNewTrackRequested(TrackType::Audio);
                if (result == 102 && onNewTrackRequested) onNewTrackRequested(TrackType::Folder);

                if (result == 3 && onSaveRequested) {
                    onSaveRequested();
                }
                if (result == 5 && onExportRequested) {
                    onExportRequested();
                }
                if (result == 6 && onExportStemsRequested) {
                    onExportStemsRequested();
                }
                if (result == 10 && onThemeManagerRequested) {
                    onThemeManagerRequested();
                }
            });
    }

    // setLoudnessTrackVisible, etc. eliminados para centralizar en TrackContainer

private:
    // isLoudnessTrackVisible, etc. eliminados

    void updateStyles() {
        if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
            auto bg = theme->getSkinColor("TOP_BAR_BG", juce::Colour(20, 22, 25));
            auto accent = theme->getSkinColor("ACCENT_COLOR", juce::Colours::orange);
            
            // Botones de modo
            viewToggleBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
            
            // Botones de panel
            pickerBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
            filesBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
            mixerBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
            rackBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));
            fxBtn.setColour(juce::TextButton::buttonColourId, bg.brighter(0.1f));

            // Actualizar el tema del menú popup
            customMenuTheme.setColour(juce::PopupMenu::highlightedBackgroundColourId, accent);
        }
    }

    void lookAndFeelChanged() override {
        updateStyles();
        repaint();
    }

    void paint(juce::Graphics& g) override {
        if (auto* theme = dynamic_cast<CustomTheme*>(&getLookAndFeel())) {
            g.fillAll(theme->getSkinColor("TOP_BAR_BG", juce::Colour(20, 22, 25)));
        } else {
            g.fillAll(juce::Colour(20, 22, 25));
        }

        // DISEÑO: Hint Panel (Zona Baja con bordes redondeados)
        g.setColour(juce::Colours::black);
        g.fillRoundedRectangle(12.0f, 48.0f, 432.0f, 36.0f, 10.0f);
    }

    void resized() override {
        auto area = getLocalBounds();

        minBtn.setBounds(12, 12, 24, 24);
        maxBtn.setBounds(36, 12, 24, 24);
        closeBtn.setBounds(60, 12, 24, 24);

        btnFile.setBounds(96, 12, 30, 24);
        btnEdit.setBounds(138, 12, 30, 24);
        btnCreate.setBounds(180, 12, 48, 24);
        btnView.setBounds(240, 12, 36, 24);
        btnOptions.setBounds(288, 12, 54, 24);
        btnTools.setBounds(354, 12, 42, 24);
        btnHelp.setBounds(408, 12, 36, 24);

        viewToggleBtn.setBounds(area.removeFromRight(150).reduced(5));

        int cx = getWidth() / 2;
        playlistToggleBtn.setBounds(cx - 463, 6, 40, 40);
        playBtn.setBounds(cx - 423, 12, 30, 30);
        stopBtn.setBounds(cx - 383, 12, 30, 30);
        timeDisplayLabel.setBounds(cx - 179, 6, 163, 40);

        recordBtn.setBounds(622, 6, 40, 40);
        bpmControl.setBounds(665, 12, 70, 30);
        metronomeBtn.setBounds(738, 12, 30, 30);

        int currentX = 950;
        pickerBtn.setBounds(currentX, 6, 55, 32); currentX += 58;
        filesBtn.setBounds(currentX, 6, 55, 32); currentX += 58;
        mixerBtn.setBounds(currentX, 6, 55, 32); currentX += 58;
        rackBtn.setBounds(currentX, 6, 55, 32); currentX += 58;
        fxBtn.setBounds(currentX, 6, 55, 32);
    }

private:
    TopMenuButton btnFile{ "FILE" };
    TopMenuButton btnEdit{ "EDIT" };
    TopMenuButton btnCreate{ "CREATE" };
    TopMenuButton btnView{ "VIEW" };
    TopMenuButton btnOptions{ "OPTIONS" };
    TopMenuButton btnTools{ "TOOLS" };
    TopMenuButton btnHelp{ "HELP" };

    juce::Label timeDisplayLabel;
    WindowControlButton minBtn{ WindowControlButton::Minimize };
    WindowControlButton maxBtn{ WindowControlButton::Maximize };
    WindowControlButton closeBtn{ WindowControlButton::Close };
    juce::ComponentDragger dragger;

    class DarkMenuTheme : public juce::LookAndFeel_V4 {
    public:
        DarkMenuTheme() {
            setColour(juce::PopupMenu::backgroundColourId, juce::Colour(35, 38, 42));
            setColour(juce::PopupMenu::textColourId, juce::Colours::white);
            setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(200, 130, 30));
            setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        }

        juce::Font getPopupMenuFont() override {
            return juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::plain);
        }

        void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override {
            g.fillAll(findColour(juce::PopupMenu::backgroundColourId));
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.drawRect(0, 0, width, height, 1);
        }

        void getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator,
                                        int standardMenuItemHeight, int& w, int& h) override {
            h = 32; // Altura fija para evitar cálculos lentos
            w = juce::Font(14.0f).getStringWidth(text) + 40;
        }
    };
    DarkMenuTheme customMenuTheme;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopMenuBar)
};