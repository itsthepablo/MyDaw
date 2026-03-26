#pragma once
#include <JuceHeader.h>

class TopMenuButton : public juce::Button {
public:
    TopMenuButton(const juce::String& name) : Button(name) {}

    void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override {
        if (isButtonDown || isMouseOverButton) {
            g.fillAll(juce::Colours::white.withAlpha(0.08f));
        }

        g.setColour(isMouseOverButton ? juce::Colours::white : juce::Colour(170, 175, 180));

        juce::Font font(juce::Font::getDefaultSansSerifFontName(), 14.0f, juce::Font::plain);
        g.setFont(font);
        g.drawFittedText(getName(), getLocalBounds(), juce::Justification::centred, 1, 0.8f);
    }
};

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
        if (type == Minimize) g.drawHorizontalLine((int)(cy + 4), cx - w / 2, cx + w / 2 + 1);
        else if (type == Maximize) g.drawRect(cx - w / 2, cy - w / 2, w, w, 1.2f);
        else if (type == Close) {
            g.drawLine(cx - w / 2, cy - w / 2, cx + w / 2, cy + w / 2, 1.2f);
            g.drawLine(cx - w / 2, cy + w / 2, cx + w / 2, cy - w / 2, 1.2f);
        }
    }
private:
    Type type;
};

class TopMenuBar : public juce::Component {
public:
    juce::TextButton viewToggleBtn;
    juce::TextButton playlistToggleBtn;
    juce::TextButton playBtn;
    juce::TextButton stopBtn;
    std::function<void()> onSaveRequested;

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

        addAndMakeVisible(minBtn);
        addAndMakeVisible(maxBtn);
        addAndMakeVisible(closeBtn);

        minBtn.onClick = [this] { if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>()) dw->setMinimised(true); };

        // =========================================================================
        // FIX BOTÓN MAXIMIZAR: Alterna entre el UserArea y un tamaño restaurado flotante.
        // =========================================================================
        maxBtn.onClick = [this] {
            if (auto* dw = findParentComponentOfClass<juce::DocumentWindow>()) {
                auto userArea = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
                if (dw->getBounds() == userArea) {
                    dw->centreWithSize(1200, 800); // Tamaño ventana flotante al restaurar
                }
                else {
                    dw->setBounds(userArea); // Maximiza sin comer bordes
                }
            }
            };

        closeBtn.onClick = [this] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); };
    }

    ~TopMenuBar() override {}

    void mouseDown(const juce::MouseEvent& e) override { dragger.startDraggingComponent(getTopLevelComponent(), e); }
    void mouseDrag(const juce::MouseEvent& e) override { dragger.dragComponent(getTopLevelComponent(), e, nullptr); }

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

        // --- ZONA IZQUIERDA: Controles de Ventana ---
        minBtn.setBounds(5, 15, 24, 24);
        maxBtn.setBounds(30, 15, 24, 24);
        closeBtn.setBounds(55, 15, 24, 24);

        // --- ZONA IZQUIERDA: Menú de Archivo ---
        btnFile.setBounds(90, 6, 40, 40);
        btnEdit.setBounds(132, 6, 40, 40);
        btnCreate.setBounds(173, 6, 54, 40);
        btnView.setBounds(226, 6, 45, 40);
        btnOptions.setBounds(272, 6, 62, 40);
        btnTools.setBounds(334, 6, 50, 40);
        btnHelp.setBounds(383, 6, 45, 40);

        // --- ZONA DERECHA: Botón Mixer ---
        viewToggleBtn.setBounds(area.removeFromRight(150).reduced(5));

        // --- ZONA CENTRAL RESPONSIVA: Transporte ---
        int cx = getWidth() / 2;
        playlistToggleBtn.setBounds(cx - 463, 6, 40, 40);
        playBtn.setBounds(cx - 423, 6, 40, 40);
        stopBtn.setBounds(cx - 383, 6, 40, 40);
        timeDisplayLabel.setBounds(cx - 179, 6, 163, 40);
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