#pragma once
#include <JuceHeader.h>
#include "CustomTheme.h"

class ThemeManagerPanel : public juce::Component
{
public:
    ThemeManagerPanel()
    {
        addAndMakeVisible(applyBtn);
        applyBtn.setButtonText("Apply & Save");
        applyBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(50, 150, 50));
        applyBtn.onClick = [this] { saveTheme(); };

        addAndMakeVisible(resetBtn);
        resetBtn.setButtonText("Reset Defaults");
        resetBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(150, 50, 50));
        resetBtn.onClick = [this] { resetDefaults(); };

        addAndMakeVisible(viewport);
        viewport.setViewedComponent(&listContainer);
        
        refreshList();
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(10);
        auto bottomArea = area.removeFromBottom(40);
        
        applyBtn.setBounds(bottomArea.removeFromRight(120));
        bottomArea.removeFromRight(10);
        resetBtn.setBounds(bottomArea.removeFromRight(120));
        
        area.removeFromBottom(10);
        viewport.setBounds(area);
        
        listContainer.setBounds(0, 0, viewport.getWidth() - 20, colorRows.size() * 40);
        for (int i = 0; i < colorRows.size(); ++i)
            colorRows[i]->setBounds(0, i * 40, listContainer.getWidth(), 35);
    }

private:
    struct ColorRow;
    inline static juce::Colour lastCopiedColor = juce::Colours::transparentBlack;

    struct ColorRow : public juce::Component, public juce::ChangeListener
    {
        ColorRow(juce::String n, juce::Colour c, std::function<void(juce::Colour)> onUpdate)
            : name(n), currentColor(c), onColorUpdate(onUpdate)
        {
            addAndMakeVisible(label);
            label.setText(name, juce::dontSendNotification);
            label.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.7f));
            
            addAndMakeVisible(colorButton);
            colorButton.setAlpha(0.0f); // Invisible button but still clickable
            colorButton.onClick = [this] {
                auto* selector = new juce::ColourSelector();
                selector->setName(name);
                selector->setCurrentColour(currentColor);
                selector->addChangeListener(this);
                selector->setSize(400, 400);

                juce::CallOutBox::launchAsynchronously(std::unique_ptr<juce::Component>(selector), 
                                                       colorButton.getScreenBounds(), nullptr);
            };

            addAndMakeVisible(copyBtn);
            copyBtn.setButtonText("C");
            copyBtn.setTooltip("Copy Color");
            copyBtn.onClick = [this] { lastCopiedColor = currentColor; };

            addAndMakeVisible(pasteBtn);
            pasteBtn.setButtonText("P");
            pasteBtn.setTooltip("Paste Color");
            pasteBtn.onClick = [this] { 
                if (!lastCopiedColor.isTransparent()) {
                    currentColor = lastCopiedColor;
                    if (onColorUpdate) onColorUpdate(currentColor);
                    repaint();
                }
            };
        }

        void changeListenerCallback(juce::ChangeBroadcaster* source) override
        {
            if (auto* selector = dynamic_cast<juce::ColourSelector*>(source))
            {
                currentColor = selector->getCurrentColour();
                repaint();
                if (onColorUpdate) onColorUpdate(currentColor);
            }
        }

        void paint(juce::Graphics& g) override
        {
            g.setColour(currentColor);
            g.fillRoundedRectangle(colorButton.getBounds().toFloat(), 4.0f);
            g.setColour(juce::Colours::white.withAlpha(0.3f));
            g.drawRoundedRectangle(colorButton.getBounds().toFloat(), 4.0f, 1.0f);
        }

        void resized() override
        {
            auto area = getLocalBounds();
            colorButton.setBounds(area.removeFromRight(60).reduced(5));
            pasteBtn.setBounds(area.removeFromRight(25).reduced(2));
            copyBtn.setBounds(area.removeFromRight(25).reduced(2));
            label.setBounds(area.reduced(5));
        }

        juce::String name;
        juce::Colour currentColor;
        std::function<void(juce::Colour)> onColorUpdate;
        juce::Label label;
        juce::TextButton colorButton;
        juce::TextButton copyBtn, pasteBtn;
    };

    void refreshList()
    {
        colorRows.clear();
        auto* theme = dynamic_cast<CustomTheme*>(&juce::LookAndFeel::getDefaultLookAndFeel());
        if (!theme) return;

        auto keys = theme->getColorKeys();
        for (auto key : keys)
        {
            auto* row = new ColorRow(key, theme->getSkinColor(key, juce::Colours::black), [this, key](juce::Colour c) {
                if (auto* t = dynamic_cast<CustomTheme*>(&juce::LookAndFeel::getDefaultLookAndFeel()))
                    t->setSkinColor(key, c, false); // Update real-time but don't save yet
            });
            colorRows.add(row);
            listContainer.addAndMakeVisible(row);
        }
        resized();
    }

    void saveTheme()
    {
        if (auto* t = dynamic_cast<CustomTheme*>(&juce::LookAndFeel::getDefaultLookAndFeel()))
            t->saveThemeToFile();
        
        if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>())
            dw->setVisible(false);
    }

    void resetDefaults()
    {
        if (auto* t = dynamic_cast<CustomTheme*>(&juce::LookAndFeel::getDefaultLookAndFeel())) {
            juce::File file = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                .getChildFile("MyDAW_Skins/theme_custom.json");
            if (file.existsAsFile()) file.deleteFile();
            t->loadThemeFromFile();
            refreshList();
        }
    }

    juce::Viewport viewport;
    juce::Component listContainer;
    juce::OwnedArray<ColorRow> colorRows;
    juce::TextButton applyBtn, resetBtn;
};
