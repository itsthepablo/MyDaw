#pragma once
#include <JuceHeader.h>
#include <functional>

class BottomDockResizer : public juce::Component {
public:
    BottomDockResizer() {
        // Cambiamos el cursor nativo para indicar redimensionamiento vertical (arriba/abajo)
        setMouseCursor(juce::MouseCursor::UpDownResizeCursor);
    }

    std::function<void(int)> onResizeDelta;

    void mouseDown(const juce::MouseEvent& e) override {
        // Guardamos la posición inicial Y exacta al hacer clic
        startY = e.getScreenY();
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        int currentY = e.getScreenY();
        
        // Calculamos cuánto se movió el ratón. 
        // Nota: Si arrastramos hacia ARRIBA, la diferencia es NEGATIVA, 
        // pero queremos que el panel crezca (más altura), por lo que invertimos el signo al usarlo.
        int deltaY = currentY - startY;
        
        if (deltaY != 0) {
            if (onResizeDelta) onResizeDelta(deltaY);
            startY = currentY; // Reiniciamos para el siguiente frame
        }
    }

    void paint(juce::Graphics& g) override {
        // Fondo muy oscuro
        g.fillAll(juce::Colour(20, 22, 25)); 
        
        // Dibujamos una sutil sombra arriba y un brillo abajo para dar efecto 3D
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawHorizontalLine(0, 0, (float)getWidth());
        
        g.setColour(juce::Colours::white.withAlpha(0.05f));
        g.drawHorizontalLine(1, 0, (float)getWidth());
    }

private:
    int startY = 0;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BottomDockResizer)
};