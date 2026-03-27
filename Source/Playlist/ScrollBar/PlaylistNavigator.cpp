#include "PlaylistNavigator.h"

PlaylistNavigator::PlaylistNavigator() {}
PlaylistNavigator::~PlaylistNavigator() {}

void PlaylistNavigator::setRangeLimits(double minimum, double maximum) {
    totalMin = minimum;
    totalMax = juce::jmax(minimum + 1.0, maximum);
    repaint();
}

void PlaylistNavigator::setCurrentRange(double newStart, double newSize) {
    visibleSize = juce::jmin(newSize, totalMax - totalMin);
    currentStart = juce::jlimit(totalMin, juce::jmax(totalMin, totalMax - visibleSize), newStart);
    repaint();
}

juce::Rectangle<int> PlaylistNavigator::getThumbBounds() const {
    double range = totalMax - totalMin;
    if (range <= 0.0) return {};
    
    double scale = getWidth() / range;
    int x = (int)((currentStart - totalMin) * scale);
    int w = juce::jmax(15, (int)(visibleSize * scale)); // Mínimo 15px de ancho para poder agarrarlo
    
    // Si la vista es más grande que el contenido, llena toda la barra
    if (visibleSize >= range) {
        x = 0;
        w = getWidth();
    }
    
    return {x, 0, w, getHeight()};
}

void PlaylistNavigator::paint(juce::Graphics& g) {
    // 1. Fondo oscuro: ¡Aquí es donde en el futuro dibujarás las miniaturas de los clips!
    g.fillAll(juce::Colour(15, 17, 20));

    // 2. Dibujar el recuadro visible (El "Thumb")
    auto thumb = getThumbBounds();
    
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.fillRect(thumb);
    
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.drawRect(thumb, 1.0f);
}

void PlaylistNavigator::mouseDown(const juce::MouseEvent& e) {
    auto thumb = getThumbBounds();
    
    if (thumb.contains(e.getPosition())) {
        isDraggingThumb = true;
        dragStartMouseX = e.position.x;
        dragStartThumbX = currentStart;
    } else {
        // Clic fuera del Thumb: Salto directo hacia esa posición
        isDraggingThumb = false;
        double range = totalMax - totalMin;
        double scale = range / getWidth();
        
        double newStart = (e.position.x * scale) + totalMin - (visibleSize / 2.0);
        newStart = juce::jlimit(totalMin, juce::jmax(totalMin, totalMax - visibleSize), newStart);
        
        if (newStart != currentStart) {
            currentStart = newStart;
            if (onScrollMoved) onScrollMoved(currentStart);
            repaint();
        }
    }
}

void PlaylistNavigator::mouseDrag(const juce::MouseEvent& e) {
    if (isDraggingThumb) {
        double range = totalMax - totalMin;
        double scale = range / getWidth();
        double deltaX = (e.position.x - dragStartMouseX) * scale;
        
        double newStart = dragStartThumbX + deltaX;
        newStart = juce::jlimit(totalMin, juce::jmax(totalMin, totalMax - visibleSize), newStart);
        
        if (newStart != currentStart) {
            currentStart = newStart;
            if (onScrollMoved) onScrollMoved(currentStart);
            repaint();
        }
    }
}