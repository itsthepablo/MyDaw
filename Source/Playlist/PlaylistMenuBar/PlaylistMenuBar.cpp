#include "PlaylistMenuBar.h"

PlaylistMenuBar::PlaylistMenuBar()
{
    // Aquí inicializaremos y agregaremos los botones con addAndMakeVisible()
}

PlaylistMenuBar::~PlaylistMenuBar()
{
}

void PlaylistMenuBar::paint(juce::Graphics& g)
{
    // Fondo base del MenuBar para que encaje con la paleta de colores del DAW
    g.fillAll(juce::Colour(35, 37, 40)); 
    
    // Borde inferior sutil para separarlo de la zona del timeline
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());
}

void PlaylistMenuBar::resized()
{
    // Aquí posicionaremos los botones internos usando getLocalBounds()
}