#pragma once
#include <JuceHeader.h>

// ==============================================================================
// BOTONES DE CONTROL DE VENTANA
// ==============================================================================
class WindowControlButton : public juce::Button {
public:
    enum Type { Minimize, Maximize, Close };

    WindowControlButton(Type t) : Button("WinCtrlBtn"), type(t) {
        setTooltip(getTooltipForType(t));
    }

    juce::String getTooltipForType(Type t) {
        if (t == Minimize) return "Minimizar ventana";
        if (t == Maximize) return "Maximizar / Restaurar ventana";
        return "Cerrar DAW";
    }

    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override {
        auto bounds = getLocalBounds().toFloat();

        if (type == Close) {
            if (isButtonDown) g.fillAll(juce::Colour(255, 100, 100));
            else if (isMouseOverButton) g.fillAll(juce::Colour(232, 17, 35));
        }
        else {
            if (isButtonDown) g.fillAll(juce::Colours::white.withAlpha(0.2f));
            else if (isMouseOverButton) g.fillAll(juce::Colours::white.withAlpha(0.1f));
        }

        g.setColour(isMouseOverButton && type == Close ? juce::Colours::white : juce::Colour(170, 175, 180));

        float cx = bounds.getCentreX();
        float cy = bounds.getCentreY();
        float w = 10.0f;

        if (type == Minimize) {
            g.drawHorizontalLine((int)(cy + 4), cx - w / 2, cx + w / 2 + 1);
        }
        else if (type == Maximize) {
            g.drawRect(cx - w / 2, cy - w / 2, w, w, 1.2f);
        }
        else if (type == Close) {
            g.drawLine(cx - w / 2, cy - w / 2, cx + w / 2, cy + w / 2, 1.2f);
            g.drawLine(cx - w / 2, cy + w / 2, cx + w / 2, cy - w / 2, 1.2f);
        }
    }
private:
    Type type;
};

// ==============================================================================
// TOP MENU BAR (LIMPIA, SIN HINT PANEL)
// ==============================================================================
class TopMenuBar : public juce::Component, public juce::MenuBarModel {
public:
    TopMenuBar() {
        menuBar.reset(new juce::MenuBarComponent(this));
        menuBar->setLookAndFeel(&customMenuTheme);
        addAndMakeVisible(menuBar.get());

        addAndMakeVisible(minBtn);
        addAndMakeVisible(maxBtn);
        addAndMakeVisible(closeBtn);

        minBtn.onClick = [this] {
            if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>())
                dw->setMinimised(true);
            };

        maxBtn.onClick = [this] {
            if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>())
                dw->setFullScreen(!dw->isFullScreen());
            };

        closeBtn.onClick = [this] {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
            };
    }

    ~TopMenuBar() override {
        menuBar->setLookAndFeel(nullptr);
    }

    void mouseDown(const juce::MouseEvent& e) override {
        dragger.startDraggingComponent(getTopLevelComponent(), e);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        dragger.dragComponent(getTopLevelComponent(), e, nullptr);
    }

    juce::StringArray getMenuBarNames() override {
        return { "FILE", "EDIT", "VIEW", "OPTIONS", "TOOLS", "HELP" };
    }

    juce::PopupMenu getMenuForIndex(int, const juce::String&) override {
        juce::PopupMenu menu;
        menu.addItem(1, "Vacío por ahora...", false, false);
        return menu;
    }

    void menuItemSelected(int, int) override {}

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(20, 22, 25));
    }

    void resized() override {
        auto area = getLocalBounds();

        // 1. Botones de ventana a la derecha
        int btnW = 45;
        closeBtn.setBounds(area.removeFromRight(btnW));
        maxBtn.setBounds(area.removeFromRight(btnW));
        minBtn.setBounds(area.removeFromRight(btnW));

        // 2. Menús a la izquierda
        area.removeFromLeft(10);
        menuBar->setBounds(area.removeFromLeft(400));
    }

private:
    std::unique_ptr<juce::MenuBarComponent> menuBar;
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