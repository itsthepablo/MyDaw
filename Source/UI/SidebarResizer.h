#pragma once
#include <JuceHeader.h>
#include <functional>

class SidebarResizer : public juce::Component {
public:
    SidebarResizer() {
        // Cambiamos el cursor nativo del sistema para indicar que es redimensionable
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    }

    std::function<void(int)> onResizeDelta;

    void mouseDown(const juce::MouseEvent& e) override {
        // Guardamos la posición inicial exacta en la pantalla al hacer clic
        startX = e.getScreenX();
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        int currentX = e.getScreenX();
        int delta = currentX - startX;
        
        if (delta != 0) {
            // Le avisamos al MainComponent cuántos píxeles nos movimos
            if (onResizeDelta) onResizeDelta(delta);
            startX = currentX; // Reiniciamos para el siguiente frame del arrastre
        }
    }

    void paint(juce::Graphics& g) override {
        // Dibujamos un fondo neutro
        g.fillAll(juce::Colour(30, 32, 35)); 
        
        // Dibujamos una sutil sombra y un pequeño brillo para que parezca un divisor 3D
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawVerticalLine(0, 0, (float)getHeight());
        
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawVerticalLine(1, 0, (float)getHeight());
    }

private:
    int startX = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidebarResizer)
};