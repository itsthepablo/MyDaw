#pragma once
#include <JuceHeader.h>

/**
 * CustomCursors: Generador de cursores para el DAW dibujados por código.
 * Asegura nitidez total y evita el uso de archivos externos.
 */
class CustomCursors
{
public:
    enum class CursorType {
        Pencil,
        Cutter,
        Eraser,
        Mute
    };

    static juce::MouseCursor getCursor (CursorType type)
    {
        const int size = 32; // Tamaño base para cursores estándar
        juce::Image img (juce::Image::ARGB, size, size, true);
        juce::Graphics g (img);
        
        juce::Path p;
        int hotspotX = 0;
        int hotspotY = 0;

        switch (type)
        {
            case CursorType::Pencil:
                p = createPencilPath (size);
                hotspotX = 0;
                hotspotY = size;
                break;

            case CursorType::Cutter:
                p = createCutterPath (size);
                hotspotX = size / 2;
                hotspotY = size / 2;
                break;

            case CursorType::Eraser:
                p = createEraserPath (size);
                hotspotX = size / 2;
                hotspotY = size / 2;
                break;

            case CursorType::Mute:
                p = createMutePath (size);
                hotspotX = size / 2;
                hotspotY = size / 2;
                break;
        }

        // Estilo visual: Relleno blanco con borde negro para máxima visibilidad en cualquier fondo
        g.setColour (juce::Colours::white);
        g.fillPath (p);
        g.setColour (juce::Colours::black);
        g.strokePath (p, juce::PathStrokeType (1.5f));

        return juce::MouseCursor (img, hotspotX, hotspotY);
    }

private:
    static juce::Path createPencilPath (int s)
    {
        juce::Path p;
        float size = (float)s;
        
        // El lápiz apunta a la esquina inferior izquierda (0, size)
        p.startNewSubPath (0.0f, size);
        p.lineTo (size * 0.25f, size * 0.75f);
        p.lineTo (size * 0.85f, size * 0.15f);
        p.lineTo (size, 0.0f);
        p.lineTo (size * 0.75f, size * 0.25f);
        p.lineTo (size * 0.15f, size * 0.85f);
        p.closeSubPath();
        
        // Detalle de la punta
        p.startNewSubPath (size * 0.12f, size * 0.88f);
        p.lineTo (size * 0.25f, size * 0.75f);
        p.lineTo (size * 0.40f, size * 0.88f);
        p.closeSubPath();
        
        return p;
    }

    static juce::Path createCutterPath (int s)
    {
        juce::Path p;
        float size = (float)s;
        
        // Forma de cuchilla (X-Acto style)
        p.startNewSubPath (size * 0.4f, size * 0.9f);
        p.lineTo (size * 0.6f, size * 0.9f);
        p.lineTo (size * 0.6f, size * 0.4f);
        p.lineTo (size * 0.9f, 0.0f);
        p.lineTo (size * 0.3f, 0.0f);
        p.lineTo (size * 0.4f, size * 0.4f);
        p.closeSubPath();
        
        return p;
    }

    static juce::Path createEraserPath (int s)
    {
        juce::Path p;
        float size = (float)s;
        
        // Forma de bloque inclinado
        p.addRoundedRectangle (size * 0.1f, size * 0.25f, size * 0.8f, size * 0.5f, 2.0f);
        
        return p;
    }

    static juce::Path createMutePath (int s)
    {
        juce::Path p;
        float size = (float)s;
        
        // Altavoz tachado
        // Cuerpo altavoz
        p.startNewSubPath (size * 0.1f, size * 0.35f);
        p.lineTo (size * 0.4f, size * 0.35f);
        p.lineTo (size * 0.65f, size * 0.15f);
        p.lineTo (size * 0.65f, size * 0.85f);
        p.lineTo (size * 0.4f, size * 0.65f);
        p.lineTo (size * 0.1f, size * 0.65f);
        p.closeSubPath();
        
        // La X de mute
        p.startNewSubPath (size * 0.75f, size * 0.4f);
        p.lineTo (size * 0.95f, size * 0.6f);
        p.startNewSubPath (size * 0.95f, size * 0.4f);
        p.lineTo (size * 0.75f, size * 0.6f);
        
        return p;
    }
};
