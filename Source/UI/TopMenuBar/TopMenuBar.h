#pragma once
#include <JuceHeader.h>
#include <cmath>
#include "TopMenuButton.h"
#include "WindowControlButton.h"
#include "BpmDragControl.h"
#include "RecordButton.h"

class TopMenuBar : public juce::Component, public juce::Timer {
public:
    juce::TextButton viewToggleBtn;
    juce::TextButton playlistToggleBtn;
    juce::TextButton playBtn;
    juce::TextButton stopBtn;
    BpmDragControl bpmControl;
    juce::TextButton metronomeBtn;
    RecordButton recordBtn;

    juce::TextButton pickerBtn{ "Pick" };
    juce::TextButton filesBtn{ "Files" };
    juce::TextButton mixerBtn{ "Mixer" };
    juce::TextButton rackBtn{ "Rack" };
    juce::TextButton fxBtn{ "Effects" };

    std::function<void()> onSaveRequested;
    std::function<double()> requestPlaybackTimeInSeconds;

    std::function<void()> onTogglePicker, onToggleFiles, onToggleMixer, onToggleRack, onToggleFx;

    TopMenuBar() {
        addAndMakeVisible(btnFile);
        addAndMakeVisible(btnEdit);
        addAndMakeVisible(btnCreate);
        addAndMakeVisible(btnView);
        addAndMakeVisible(btnOptions);
        addAndMakeVisible(btnTools);
        addAndMakeVisible(btnHelp);

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
        playBtn.setButtonText("P");
        playBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::green);

        addAndMakeVisible(stopBtn);
        stopBtn.setButtonText(u8"\u25B2");
        stopBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::red);

        addAndMakeVisible(timeDisplayLabel);
        timeDisplayLabel.setText("00:00:00", juce::dontSendNotification);
        timeDisplayLabel.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.5f));
        timeDisplayLabel.setColour(juce::Label::textColourId, juce::Colours::greenyellow);
        timeDisplayLabel.setJustificationType(juce::Justification::centred);
        timeDisplayLabel.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 24.0f, juce::Font::bold));
        timeDisplayLabel.setBorderSize(juce::BorderSize<int>(2));

        addAndMakeVisible(bpmControl);

        addAndMakeVisible(metronomeBtn);
        metronomeBtn.setButtonText("MET");
        metronomeBtn.setClickingTogglesState(true);
        metronomeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(40, 45, 50));
        metronomeBtn.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);

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

        startTimerHz(30);
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
        }
        else {
            menu.addItem(1, "Vacio por ahora...", false, false);
        }

        menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&targetButton),
            [this](int result) {
                if (result == 3 && onSaveRequested) {
                    onSaveRequested();
                }
            });
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(20, 22, 25));
    }

    void resized() override {
        auto area = getLocalBounds();

        minBtn.setBounds(5, 15, 24, 24);
        maxBtn.setBounds(30, 15, 24, 24);
        closeBtn.setBounds(55, 15, 24, 24);

        btnFile.setBounds(90, 6, 40, 40);
        btnEdit.setBounds(132, 6, 40, 40);
        btnCreate.setBounds(173, 6, 54, 40);
        btnView.setBounds(226, 6, 45, 40);
        btnOptions.setBounds(272, 6, 62, 40);
        btnTools.setBounds(334, 6, 50, 40);
        btnHelp.setBounds(383, 6, 45, 40);

        viewToggleBtn.setBounds(area.removeFromRight(150).reduced(5));

        int cx = getWidth() / 2;
        playlistToggleBtn.setBounds(cx - 463, 6, 40, 40);
        playBtn.setBounds(cx - 423, 6, 40, 40);
        stopBtn.setBounds(cx - 383, 6, 40, 40);
        timeDisplayLabel.setBounds(cx - 179, 6, 163, 40);

        recordBtn.setBounds(622, 6, 40, 40);
        bpmControl.setBounds(665, 12, 70, 30);
        metronomeBtn.setBounds(738, 6, 40, 40);

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
    };
    DarkMenuTheme customMenuTheme;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopMenuBar)
};