#include "VerticalNavigator.h"

VerticalNavigator::VerticalNavigator() {}
VerticalNavigator::~VerticalNavigator() {}

void VerticalNavigator::setRangeLimits(double minimum, double maximum) {
    totalMin = minimum; totalMax = juce::jmax(minimum + 1.0, maximum); repaint();
}

void VerticalNavigator::setCurrentRange(double newStart, double newSize) {
    visibleSize = juce::jmin(newSize, totalMax - totalMin);
    currentStart = juce::jlimit(totalMin, juce::jmax(totalMin, totalMax - visibleSize), newStart);
    repaint();
}

juce::Rectangle<int> VerticalNavigator::getUpButtonBounds() const { return { 0, 0, getWidth(), btnSize }; }
juce::Rectangle<int> VerticalNavigator::getDownButtonBounds() const { return { 0, getHeight() - btnSize, getWidth(), btnSize }; }
juce::Rectangle<int> VerticalNavigator::getTrackBounds() const { return { 0, btnSize, getWidth(), getHeight() - (btnSize * 2) }; }

juce::Rectangle<int> VerticalNavigator::getThumbBounds() const {
    auto track = getTrackBounds();
    double range = totalMax - totalMin;
    if (range <= 0.0) return track;

    double scale = track.getHeight() / range;
    int y = track.getY() + (int)((currentStart - totalMin) * scale);
    int h = juce::jmax(15, (int)(visibleSize * scale));

    if (visibleSize >= range) { y = track.getY(); h = track.getHeight(); }
    return { 0, y, getWidth(), h };
}

void VerticalNavigator::paint(juce::Graphics& g) {
    bool isActive = (totalMax - totalMin) > visibleSize;

    g.fillAll(juce::Colour(15, 17, 20));

    g.setColour(juce::Colour(30, 32, 35));
    g.fillRect(getUpButtonBounds());
    g.fillRect(getDownButtonBounds());

    g.setColour(juce::Colours::white.withAlpha(isActive ? 0.5f : 0.1f));
    float cx = getWidth() / 2.0f;
    juce::Path pU, pD;
    pU.addTriangle(cx - 4.0f, 14.0f, cx + 4.0f, 14.0f, cx, 6.0f);
    g.fillPath(pU);
    pD.addTriangle(cx - 4.0f, getHeight() - 14.0f, cx + 4.0f, getHeight() - 14.0f, cx, getHeight() - 6.0f);
    g.fillPath(pD);

    if (isActive) {
        auto thumb = getThumbBounds();
        g.setColour(juce::Colours::white.withAlpha(0.15f));
        g.fillRect(thumb);
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.drawRect(thumb, 1.0f);
    }
}

void VerticalNavigator::mouseDown(const juce::MouseEvent& e) {
    bool isActive = (totalMax - totalMin) > visibleSize;
    if (!isActive) return;

    if (getUpButtonBounds().contains(e.getPosition())) { dragMode = HoldingUpBtn; startTimer(30); return; }
    if (getDownButtonBounds().contains(e.getPosition())) { dragMode = HoldingDownBtn; startTimer(30); return; }

    auto thumb = getThumbBounds();
    if (thumb.contains(e.getPosition())) {
        dragMode = DraggingThumb;
        dragStartMouseY = e.position.y;
        dragStartThumbY = currentStart;
    }
    else if (getTrackBounds().contains(e.getPosition())) {
        // --- LA MAGIA: Caminamos hacia el ratón ---
        targetMouseY = e.position.y;
        if (e.position.y < thumb.getY()) {
            dragMode = HoldingUpBtn;
            startTimer(30);
        }
        else if (e.position.y > thumb.getBottom()) {
            dragMode = HoldingDownBtn;
            startTimer(30);
        }
    }
}

void VerticalNavigator::mouseDrag(const juce::MouseEvent& e) {
    if (dragMode == DraggingThumb) {
        double scale = (totalMax - totalMin) / getTrackBounds().getHeight();
        double deltaY = (e.position.y - dragStartMouseY) * scale;
        double newStart = juce::jlimit(totalMin, juce::jmax(totalMin, totalMax - visibleSize), dragStartThumbY + deltaY);
        if (newStart != currentStart) {
            currentStart = newStart;
            if (onScrollMoved) onScrollMoved(currentStart);
            repaint();
        }
    }
}

void VerticalNavigator::mouseUp(const juce::MouseEvent&) {
    dragMode = None;
    targetMouseY = -1; // Limpiamos
    stopTimer();
}

void VerticalNavigator::timerCallback() {
    // --- SEGURO ANTI-DESBORDAMIENTO VERTICAL ---
    if (targetMouseY != -1) {
        auto thumb = getThumbBounds();
        if (targetMouseY >= thumb.getY() && targetMouseY <= thumb.getBottom()) {
            stopTimer(); // Se detiene cuando alcanza el ratón
            return;
        }
    }

    double step = 20.0;
    double newStart = currentStart;
    if (dragMode == HoldingUpBtn) newStart -= step;
    else if (dragMode == HoldingDownBtn) newStart += step;

    newStart = juce::jlimit(totalMin, juce::jmax(totalMin, totalMax - visibleSize), newStart);
    if (newStart != currentStart) {
        currentStart = newStart;
        if (onScrollMoved) onScrollMoved(currentStart);
        repaint();
    }
}