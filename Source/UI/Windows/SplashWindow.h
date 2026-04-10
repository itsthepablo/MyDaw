#pragma once
#include <JuceHeader.h>

// --- COMPONENTE INTERNO DEL SPLASH (CARGA IMAGEN EXTERNA DINÁMICA) ---
class SplashContentComponent : public juce::Component
{
public:
    SplashContentComponent()
    {
        setSize(600, 400);

        // 1. OBTENEMOS LA RUTA DINÁMICA DEL SISTEMA OPERATIVO
        juce::File docsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);

        // 2. CONSTRUIMOS LA RUTA HACIA TU CARPETA DE SKINS
        // Usamos CharPointer_UTF8 para evitar errores de validacion ASCII al cambiar nombres
        juce::File skinDir = docsDir.getChildFile(juce::String(juce::CharPointer_UTF8("MyDAW_Skins")));
        juce::File imageFile = skinDir.getChildFile(juce::String(juce::CharPointer_UTF8("Splash_Screen")))
            .getChildFile(juce::String(juce::CharPointer_UTF8("splash.png")));

        // 3. CARGAMOS LA IMAGEN EN MEMORIA
        splashImage = juce::ImageCache::getFromFile(imageFile);
    }

    void paint(juce::Graphics& g) override
    {
        if (splashImage.isValid())
        {
            g.drawImage(splashImage, getLocalBounds().toFloat());
        }
        else
        {
            juce::File docsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
            juce::File skinDir = docsDir.getChildFile(juce::String(juce::CharPointer_UTF8("MyDAW_Skins")));

            juce::String errorMsg = juce::String(juce::CharPointer_UTF8("ERROR: No se encontro la imagen del Splash.\n\n"));
            errorMsg << juce::String(juce::CharPointer_UTF8("Por favor, coloca un archivo llamado 'splash.png' en:\n"));
            errorMsg << skinDir.getChildFile(juce::String(juce::CharPointer_UTF8("Splash_Screen"))).getFullPathName();

            g.fillAll(juce::Colours::darkred);
            g.setColour(juce::Colours::white);
            g.setFont(16.0f);
            g.drawText(errorMsg, getLocalBounds().reduced(20), juce::Justification::centred, true);
        }
    }
private:
    juce::Image splashImage;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashContentComponent)
};

// --- CLASE DE LA VENTANA TEMPORAL DEL SPLASH SCREEN ---
class SplashWindow : public juce::DocumentWindow, private juce::Timer
{
public:
    SplashWindow(std::function<void()> onFinished)
        : DocumentWindow(juce::String(juce::CharPointer_UTF8("Splash")), juce::Colours::black, 0),
        onSplashFinished(std::move(onFinished))
    {
        setUsingNativeTitleBar(false);
        setTitleBarHeight(0);
        setResizable(false, false);
        setDropShadowEnabled(true);

        setContentOwned(new SplashContentComponent(), true);

        centreWithSize(600, 400);
        setVisible(true);

        startTimer(3000);
    }

    void closeButtonPressed() override {}

    void timerCallback() override
    {
        stopTimer();
        if (onSplashFinished)
            onSplashFinished();
    }

private:
    std::function<void()> onSplashFinished;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashWindow)
};