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
        // Buscará: Documentos -> MyDAW_Skins -> Splash_Screen -> splash.png
        juce::File skinDir = docsDir.getChildFile("MyDAW_Skins");
        juce::File imageFile = skinDir.getChildFile("Splash_Screen").getChildFile("splash.png");
        
        // 3. CARGAMOS LA IMAGEN EN MEMORIA
        splashImage = juce::ImageCache::getFromFile(imageFile);
    }
    
    void paint(juce::Graphics& g) override
    {
        if (splashImage.isValid())
        {
            // Dibuja la imagen para que llene el espacio de la ventana
            g.drawImage(splashImage, getLocalBounds().toFloat());
        }
        else
        {
            // ALERTA VISUAL CON LA NUEVA RUTA DINÁMICA PARA DEPURACIÓN
            juce::File docsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
            juce::File skinDir = docsDir.getChildFile("MyDAW_Skins");
            
            juce::String errorMsg = "ERROR: No se encontro la imagen del Splash.\n\n";
            errorMsg << "Por favor, coloca un archivo llamado 'splash.png' en:\n";
            errorMsg << skinDir.getChildFile("Splash_Screen").getFullPathName();

            g.fillAll(juce::Colours::darkred);
            g.setColour(juce::Colours::white);
            g.setFont(16.0f);
            g.drawText(errorMsg, getLocalBounds().reduced(20), juce::Justification::centred, true);
        }
    }
private:
    juce::Image splashImage; // Memoria caché de la imagen
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashContentComponent)
};

// --- CLASE DE LA VENTANA TEMPORAL DEL SPLASH SCREEN ---
class SplashWindow : public juce::DocumentWindow, private juce::Timer
{
public:
    SplashWindow(std::function<void()> onFinished)
        : DocumentWindow("Splash", juce::Colours::black, 0),
          onSplashFinished(std::move(onFinished))
    {
        // Limpiamos los bordes del sistema operativo
        setUsingNativeTitleBar(false);
        setTitleBarHeight(0); 
        setResizable(false, false);
        setDropShadowEnabled(true);
        
        // Asignamos el componente visual que creamos arriba
        setContentOwned(new SplashContentComponent(), true);
        
        centreWithSize(600, 400);
        setVisible(true);
        
        // Arranca la cuenta regresiva de 3000 ms (3 segundos)
        startTimer(3000); 
    }
    
    void closeButtonPressed() override {} // Evita que el usuario la cierre manualmente
    
    void timerCallback() override
    {
        stopTimer();
        // Llamado asíncrono seguro para destruir esta ventana y abrir la principal
        if (onSplashFinished)
            juce::MessageManager::callAsync(onSplashFinished);
    }

private:
    std::function<void()> onSplashFinished;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SplashWindow)
};