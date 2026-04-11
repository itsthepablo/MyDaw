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
    bool canPan = (totalMax - totalMin) > visibleSize;

    g.fillAll(juce::Colour(15, 17, 20));

    if (onDrawMinimap) {
        onDrawMinimap(g, getTrackBounds());
    }

    auto trackB = getTrackBounds();
    auto thumbB = getThumbBounds();

    g.setColour(juce::Colours::black.withAlpha(0.6f));
    g.fillRect(trackB.getX(), trackB.getY(), thumbB.getX() - trackB.getX(), trackB.getHeight());
    g.fillRect(thumbB.getRight(), trackB.getY(), trackB.getRight() - thumbB.getRight(), trackB.getHeight());

    g.setColour(juce::Colour(30, 32, 35));
    g.fillRect(getLeftButtonBounds());
    g.fillRect(getRightButtonBounds());

    g.setColour(juce::Colours::white.withAlpha(canPan ? 0.5f : 0.1f));
    float cy = getHeight() / 2.0f;
    juce::Path pL, pR;
    pL.addTriangle(14.0f, cy - 4.0f, 14.0f, cy + 4.0f, 6.0f, cy);
    g.fillPath(pL);
    pR.addTriangle(getWidth() - 14.0f, cy - 4.0f, getWidth() - 14.0f, cy + 4.0f, getWidth() - 6.0f, cy);
    g.fillPath(pR);

    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.fillRect(thumbB);
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawRect(thumbB, 1.0f);

    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.fillRect(thumbB.getX() + 2, getHeight() / 2 - 4, 2, 8);
    g.fillRect(thumbB.getRight() - 4, getHeight() / 2 - 4, 2, 8);
}

void PlaylistNavigator::mouseDown(const juce::MouseEvent& e) {
    bool canPan = (totalMax - totalMin) > visibleSize;

    if (getLeftButtonBounds().contains(e.getPosition())) {
        if (canPan) { dragMode = HoldingLeftBtn; startTimer(30); }
        return;
    }
    if (getRightButtonBounds().contains(e.getPosition())) {
        if (canPan) { dragMode = HoldingRightBtn; startTimer(30); }
        return;
    }

    auto thumb = getThumbBounds();
    int edgeZone = 8;

    if (thumb.contains(e.getPosition())) {
        if (e.x <= thumb.getX() + edgeZone) dragMode = DraggingLeftEdge;
        else if (e.x >= thumb.getRight() - edgeZone) dragMode = DraggingRightEdge;
        else if (canPan) dragMode = DraggingThumb;
        else dragMode = None;

        dragStartMouseX = e.position.x;
        dragStartThumbX = currentStart;
        dragStartZoom = currentZoom;
        dragStartThumbLeft = thumb.getX();
        dragStartThumbRight = thumb.getRight();
    }
    else if (getTrackBounds().contains(e.getPosition()) && canPan) {
        // --- LA MAGIA: En lugar de saltar directo, caminamos hacia el ratón ---
        targetMouseX = e.position.x;
        if (e.position.x < thumb.getX()) {
            dragMode = HoldingLeftBtn;
            startTimer(30);
        }
        else if (e.position.x > thumb.getRight()) {
            dragMode = HoldingRightBtn;
            startTimer(30);
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
    else if (dragMode == DraggingRightEdge) {
        int newThumbRight = juce::jlimit(dragStartThumbLeft + 15, getTrackBounds().getRight(), (int)e.position.x);
        int newThumbWidth = newThumbRight - dragStartThumbLeft;

        double newZoom = (getTrackBounds().getWidth() * visibleSize) / (newThumbWidth * baseTotalWidth);
        newZoom = juce::jlimit(0.01, 71960.0, newZoom);

        double leftTime = dragStartThumbX / dragStartZoom;
        double newStart = leftTime * newZoom;

        if (newZoom != currentZoom) {
            currentStart = newStart;
            if (onZoomChanged) onZoomChanged(newZoom, newStart);
        }
    }
    else if (dragMode == DraggingLeftEdge) {
        int newThumbLeft = juce::jlimit(getTrackBounds().getX(), dragStartThumbRight - 15, (int)e.position.x);
        int newThumbWidth = dragStartThumbRight - newThumbLeft;

        double newZoom = (getTrackBounds().getWidth() * visibleSize) / (newThumbWidth * baseTotalWidth);
        newZoom = juce::jlimit(0.01, 71960.0, newZoom);

        double rightTime = (dragStartThumbX + visibleSize) / dragStartZoom;
        double newStart = (rightTime * newZoom) - visibleSize;

        if (newZoom != currentZoom) {
            currentStart = newStart;
            if (onZoomChanged) onZoomChanged(newZoom, newStart);
        }
    }
}

void PlaylistNavigator::mouseUp(const juce::MouseEvent&) {
    dragMode = None;
    targetMouseX = -1; // Limpiamos la memoria
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
    // --- SEGURO ANTI-DESBORDAMIENTO ---
    if (targetMouseX != -1) {
        auto thumb = getThumbBounds();
        if (targetMouseX >= thumb.getX() && targetMouseX <= thumb.getRight()) {
            stopTimer(); // Se detiene justo cuando el bloque toca el ratón
            return;
        }
    }

    double step = 20.0;
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