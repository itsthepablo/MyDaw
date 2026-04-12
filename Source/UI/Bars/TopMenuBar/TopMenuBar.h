#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <functional>
#include "TopMenuButton.h"
#include "WindowControlButton.h"
#include "BpmDragControl.h"
#include "RecordButton.h"
#include "SvgToggleButton.h"
#include "../../../Theme/CustomTheme.h"
#include "../../../Data/Track.h"

/**
 * TopMenuBar — Barra superior principal con menús y controles globales de transporte.
 * Refactorizado a .cpp para mejorar tiempos de compilación.
 */
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

    TopMenuBar();
    ~TopMenuBar() override;

    void timerCallback() override;
    void setZoomInfo(double hZoom);

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void showDropdownMenu(const juce::String& menuName, juce::Button& targetButton);

    void updateStyles();
    void lookAndFeelChanged() override { updateStyles(); repaint(); }
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    TopMenuButton btnFile{ "FILE" }, btnEdit{ "EDIT" }, btnCreate{ "CREATE" }, btnView{ "VIEW" }, 
                   btnOptions{ "OPTIONS" }, btnTools{ "TOOLS" }, btnHelp{ "HELP" };

    juce::Label timeDisplayLabel;
    juce::Label zoomDisplayLabel;
    WindowControlButton minBtn{ WindowControlButton::Minimize }, 
                        maxBtn{ WindowControlButton::Maximize }, 
                        closeBtn{ WindowControlButton::Close };
    juce::ComponentDragger dragger;

    class DarkMenuTheme : public juce::LookAndFeel_V4 {
    public:
        DarkMenuTheme() {
            setColour(juce::PopupMenu::backgroundColourId, juce::Colour(35, 38, 42));
            setColour(juce::PopupMenu::textColourId, juce::Colours::white);
            setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(200, 130, 30));
            setColour(juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        }
        juce::Font getPopupMenuFont() override { return juce::Font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::plain); }
        void drawPopupMenuBackground(juce::Graphics& g, int width, int height) override {
            g.fillAll(findColour(juce::PopupMenu::backgroundColourId));
            g.setColour(juce::Colours::white.withAlpha(0.1f));
            g.drawRect(0, 0, width, height, 1);
        }
        void getIdealPopupMenuItemSize(const juce::String& text, bool, int, int& w, int& h) override {
            h = 32; w = juce::Font(14.0f).getStringWidth(text) + 40;
        }
    };
    DarkMenuTheme customMenuTheme;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopMenuBar)
};