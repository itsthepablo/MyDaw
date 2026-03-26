#include <JuceHeader.h>
#include "MainComponent.h"
#include "Theme/CustomTheme.h" 
#include "UI/SplashWindow.h"
#include "FileAssociationHandler.h"

class MyPianoRoll_AppApplication : public juce::JUCEApplication
{
public:
    MyPianoRoll_AppApplication() {}
    const juce::String getApplicationName() override { return ProjectInfo::projectName; }
    const juce::String getApplicationVersion() override { return ProjectInfo::versionString; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String& commandLine) override {
        juce::LookAndFeel::setDefaultLookAndFeel(&myCustomTheme);

        // ESCUDO ANTI-ESCALADO
        auto* display = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay();
        if (display != nullptr) {
            float osScale = (float)display->scale;
            juce::Desktop::getInstance().setGlobalScaleFactor(1.0f / osScale);
        }

        initialFileToLoad = commandLine.unquoted();
        FileAssociationHandler::registerDAWExtension();

        splashWindow.reset(new SplashWindow([this]()
            {
                juce::MessageManager::callAsync([this]()
                    {
                        mainWindow.reset(new MainWindow(getApplicationName(), initialFileToLoad));
                        splashWindow = nullptr;
                    });
            }));
    }

    void shutdown() override {
        splashWindow = nullptr;
        mainWindow = nullptr;
        juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
    }

    void systemRequestedQuit() override { quit(); }

    void anotherInstanceStarted(const juce::String& commandLine) override {
        if (mainWindow != nullptr) {
            if (auto* mc = dynamic_cast<MainComponent*>(mainWindow->getContentComponent())) {
                mc->loadProject(juce::File(commandLine.unquoted()));
            }
        }
    }

    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow(juce::String name, juce::String fileToLoad)
            : DocumentWindow(name, juce::Colours::darkgrey, DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(false);
            setTitleBarHeight(0);

            auto* mc = new MainComponent();
            setContentOwned(mc, true);

            if (fileToLoad.isNotEmpty()) {
                mc->loadProject(juce::File(fileToLoad));
            }

            setResizable(true, true);

            // =========================================================================
            // FIX REAL AL BORDE FANTASMA: Usar el UserArea de la pantalla.
            // Ocupa el 100% de la pantalla respetando la barra de tareas de Windows.
            // =========================================================================
            auto userArea = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->userArea;
            setBounds(userArea);

            setVisible(true);
        }

        void closeButtonPressed() override { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };

private:
    std::unique_ptr<SplashWindow> splashWindow;
    std::unique_ptr<MainWindow> mainWindow;
    CustomTheme myCustomTheme;
    juce::String initialFileToLoad;
};

START_JUCE_APPLICATION(MyPianoRoll_AppApplication)