#include <JuceHeader.h>
#include "MainComponent.h"
#include "Theme/CustomTheme.h" 

// --- 1. NUESTRA BARRA DE TÍTULO 100% INMUNE A WINDOWS ---
class CustomHeader : public juce::Component {
public:
    CustomHeader(juce::DocumentWindow* w) : window(w) {
        // Botón Minimizar
        addAndMakeVisible(minBtn);
        minBtn.setButtonText("-");
        minBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 54));
        minBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::gold);
        minBtn.onClick = [this] { window->setMinimised(true); };

        // Botón Maximizar
        addAndMakeVisible(maxBtn);
        maxBtn.setButtonText(juce::CharPointer_UTF8("\xe2\x96\xa1"));
        maxBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 54));
        maxBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::limegreen);
        maxBtn.onClick = [this] { window->setFullScreen(!window->isFullScreen()); };

        // Botón Cerrar
        addAndMakeVisible(closeBtn);
        closeBtn.setButtonText("x");
        closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(150, 40, 40));
        closeBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);

        // CORRECCIÓN: Se añadió 'juce::'
        closeBtn.onClick = [] { juce::JUCEApplication::getInstance()->systemRequestedQuit(); };
    }

    void paint(juce::Graphics& g) override {
        // Dibujamos el fondo oscuro y la línea separadora
        g.fillAll(juce::Colour(20, 22, 25));
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawHorizontalLine(getHeight() - 1, 0, (float)getWidth());

        // Texto del título en el centro
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(juce::Font("Sans-Serif", 15.0f, juce::Font::bold));
        g.drawText(window->getName(), getLocalBounds(), juce::Justification::centred);
    }

    void resized() override {
        // Acomodamos a la izquierda en orden: - ❐ x
        int bs = getHeight() - 8;
        int x = 8;
        int y = 4;
        minBtn.setBounds(x, y, bs, bs); x += bs + 4;
        maxBtn.setBounds(x, y, bs, bs); x += bs + 4;
        closeBtn.setBounds(x, y, bs, bs);
    }

    // ¡MAGIA! Permitimos arrastrar la ventana desde esta barra
    void mouseDown(const juce::MouseEvent& e) override { dragger.startDraggingComponent(window, e); }
    void mouseDrag(const juce::MouseEvent& e) override { dragger.dragComponent(window, e, nullptr); }

private:
    juce::DocumentWindow* window;
    juce::TextButton minBtn, maxBtn, closeBtn;
    juce::ComponentDragger dragger; // Motor de arrastre de JUCE
};

// --- 2. CONTENEDOR QUE UNE LA BARRA CON TU DAW ---
class AppContainer : public juce::Component {
public:
    AppContainer(juce::DocumentWindow* w) : header(w) {
        addAndMakeVisible(mainApp); // Tu DAW
        addAndMakeVisible(header);  // La barra superior
    }
    void resized() override {
        header.setBounds(0, 0, getWidth(), 30); // 30px para la barra
        mainApp.setBounds(0, 30, getWidth(), getHeight() - 30); // El resto para el DAW
    }
private:
    CustomHeader header;
    MainComponent mainApp;
};

// --- 3. LA VENTANA PRINCIPAL DEL PROGRAMA ---
class MyPianoRoll_AppApplication : public juce::JUCEApplication
{
public:
    MyPianoRoll_AppApplication() {}
    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override {
        juce::LookAndFeel::setDefaultLookAndFeel(&myCustomTheme);
        mainWindow.reset(new MainWindow(getApplicationName()));
    }

    void shutdown() override {
        mainWindow = nullptr;
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted(const juce::String& commandLine) override {}

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name)
            : DocumentWindow(name, juce::Colours::darkgrey, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(false);

            // ¡LA CLAVE! Le decimos a JUCE que la barra oficial mide 0 píxeles. 
            // Esto aniquila cualquier interferencia de Windows o JUCE.
            setTitleBarHeight(0);

            // Metemos nuestro contenedor maestro
            setContentOwned(new AppContainer(this), true);

#if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
#else
            setResizable(true, true);

            // --- NUEVO: Ocupar toda la pantalla respetando la barra de tareas ---
            auto screenArea = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
            setBounds(screenArea);
            // -------------------------------------------------------------------
#endif

            setVisible(true);
        }

        // CORRECCIÓN: Se añadió 'juce::'
        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }
    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    CustomTheme myCustomTheme;
};

START_JUCE_APPLICATION(MyPianoRoll_AppApplication)