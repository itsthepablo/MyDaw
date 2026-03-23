#include <JuceHeader.h>
#include "MainComponent.h"
#include "Theme/CustomTheme.h" 

// --- LA VENTANA PRINCIPAL DEL PROGRAMA ---
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
            // 1. Desactivamos la barra de Windows/Mac
            setUsingNativeTitleBar(false);

            // 2. ¡LA CLAVE! Reducimos la barra antigua a 0 píxeles.
            setTitleBarHeight(0);

            // 3. Cargamos directamente el MainComponent (que ahora tiene tu nueva TopMenuBar)
            setContentOwned(new MainComponent(), true);

#if JUCE_IOS || JUCE_ANDROID
            setFullScreen(true);
#else
            setResizable(true, true);

            // Ocupar toda la pantalla respetando la barra de tareas
            auto screenArea = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
            setBounds(screenArea);
#endif

            setVisible(true);
        }

        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<MainWindow> mainWindow;
    CustomTheme myCustomTheme;
};

START_JUCE_APPLICATION(MyPianoRoll_AppApplication)