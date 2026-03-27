#include "PlaylistNavigator.h"

PlaylistNavigator::PlaylistNavigator() {}
PlaylistNavigator::~PlaylistNavigator() {}

void PlaylistNavigator::setRangeLimits(double minimum, double maximum) {
    totalMin = minimum; totalMax = juce::jmax(minimum + 1.0, maximum); repaint();
}

void PlaylistNavigator::setCurrentRange(double newStart, double newSize) {
    visibleSize = juce::jmin(newSize, totalMax - totalMin);
    currentStart = juce::jlimit(totalMin, juce::jmax(totalMin, totalMax - visibleSize), newStart);
    repaint();
}

void PlaylistNavigator::setZoomContext(double cZoom, double bWidth) {
    currentZoom = cZoom; baseTotalWidth = bWidth;
}

juce::Rectangle<int> PlaylistNavigator::getLeftButtonBounds() const { return { 0, 0, btnSize, getHeight() }; }
juce::Rectangle<int> PlaylistNavigator::getRightButtonBounds() const { return { getWidth() - btnSize, 0, btnSize, getHeight() }; }
juce::Rectangle<int> PlaylistNavigator::getTrackBounds() const { return { btnSize, 0, getWidth() - (btnSize * 2), getHeight() }; }

juce::Rectangle<int> PlaylistNavigator::getThumbBounds() const {
    auto track = getTrackBounds();
    double range = totalMax - totalMin;
    if (range <= 0.0) return track;

    double scale = track.getWidth() / range;
    int x = track.getX() + (int)((currentStart - totalMin) * scale);
    int w = juce::jmax(15, (int)(visibleSize * scale));

    if (visibleSize >= range) { x = track.getX(); w = track.getWidth(); }
    return { x, 0, w, getHeight() };
}

void PlaylistNavigator::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(15, 17, 20));

    // Dibujar botones
    g.setColour(juce::Colour(30, 32, 35));
    g.fillRect(getLeftButtonBounds());
    g.fillRect(getRightButtonBounds());

    g.setColour(juce::Colours::white.withAlpha(0.5f));
    float cy = getHeight() / 2.0f;
    juce::Path pL, pR;
    pL.addTriangle(14.0f, cy - 4.0f, 14.0f, cy + 4.0f, 6.0f, cy);
    g.fillPath(pL);
    pR.addTriangle(getWidth() - 14.0f, cy - 4.0f, getWidth() - 14.0f, cy + 4.0f, getWidth() - 6.0f, cy);
    g.fillPath(pR);

    // Dibujar Thumb
    auto thumb = getThumbBounds();
    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.fillRect(thumb);
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawRect(thumb, 1.0f);

    // Grips visuales de Zoom en los bordes
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.fillRect(thumb.getX() + 2, getHeight() / 2 - 4, 2, 8);
    g.fillRect(thumb.getRight() - 4, getHeight() / 2 - 4, 2, 8);
}

void PlaylistNavigator::mouseDown(const juce::MouseEvent& e) {
    if (getLeftButtonBounds().contains(e.getPosition())) {
        dragMode = HoldingLeftBtn; startTimer(30); return;
    }
    if (getRightButtonBounds().contains(e.getPosition())) {
        dragMode = HoldingRightBtn; startTimer(30); return;
    }

    auto thumb = getThumbBounds();
    int edgeZone = 8;

    if (thumb.contains(e.getPosition())) {
        if (e.x <= thumb.getX() + edgeZone) dragMode = DraggingLeftEdge;
        else if (e.x >= thumb.getRight() - edgeZone) dragMode = DraggingRightEdge;
        else dragMode = DraggingThumb;

        dragStartMouseX = e.position.x;
        dragStartThumbX = currentStart;
        dragStartZoom = currentZoom;
    }
    else if (getTrackBounds().contains(e.getPosition())) {
        double range = totalMax - totalMin;
        double scale = range / getTrackBounds().getWidth();
        double newStart = ((e.position.x - btnSize) * scale) + totalMin - (visibleSize / 2.0);
        newStart = juce::jlimit(totalMin, juce::jmax(totalMin, totalMax - visibleSize), newStart);
        if (newStart != currentStart) {
            currentStart = newStart;
            if (onScrollMoved) onScrollMoved(currentStart);
            repaint();
        }
    }
}

void PlaylistNavigator::mouseDrag(const juce::MouseEvent& e) {
    if (dragMode == DraggingThumb) {
        double scale = (totalMax - totalMin) / getTrackBounds().getWidth();
        double deltaX = (e.position.x - dragStartMouseX) * scale;
        double newStart = juce::jlimit(totalMin, juce::jmax(totalMin, totalMax - visibleSize), dragStartThumbX + deltaX);
        if (newStart != currentStart) {
            currentStart = newStart;
            if (onScrollMoved) onScrollMoved(currentStart);
            repaint();
        }
    }
    else if (dragMode == DraggingRightEdge || dragMode == DraggingLeftEdge) {
        // Lógica de Zoom FL Studio
        double sensitivity = 0.005;
        double delta = (e.position.x - dragStartMouseX) * sensitivity;
        double newZoom = dragStartZoom;

        if (dragMode == DraggingRightEdge) newZoom -= delta;
        else newZoom += delta;

        newZoom = juce::jlimit(0.1, 5.0, newZoom);
        if (newZoom != currentZoom && onZoomChanged) onZoomChanged(newZoom);
    }
}

void PlaylistNavigator::mouseUp(const juce::MouseEvent&) {
    dragMode = None;
    stopTimer();
}

void PlaylistNavigator::mouseMove(const juce::MouseEvent& e) {
    auto thumb = getThumbBounds();
    if (thumb.contains(e.getPosition())) {
        if (e.x <= thumb.getX() + 8 || e.x >= thumb.getRight() - 8)
            setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
        else
            setMouseCursor(juce::MouseCursor::NormalCursor);
    }
    else {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    }
}

void PlaylistNavigator::timerCallback() {
    double step = 20.0; // Movimiento milimetrico
    double newStart = currentStart;
    if (dragMode == HoldingLeftBtn) newStart -= step;
    else if (dragMode == HoldingRightBtn) newStart += step;

    newStart = juce::jlimit(totalMin, juce::jmax(totalMin, totalMax - visibleSize), newStart);
    if (newStart != currentStart) {
        currentStart = newStart;
        if (onScrollMoved) onScrollMoved(currentStart);
        repaint();
    }
}