#include "AutomationEditor.h"
#include "PianoRollComponent.h" 

AutomationEditor::AutomationEditor() {
    addAndMakeVisible(autoSelector);
    autoSelector.addItem("VOL (Volume)", 1); autoSelector.addItem("PAN (Panning)", 2);
    autoSelector.addItem("FILTER (Cutoff)", 3); autoSelector.addItem("PITCH (Fine Tune)", 4);
    autoSelector.setSelectedId(1); autoSelector.onChange = [this] { repaint(); };

    pencilBtn.setButtonText("PENCIL"); lineBtn.setButtonText("S-CURVE"); arcBtn.setButtonText("ARC");
    sawBtn.setButtonText("SAW"); triBtn.setButtonText("TRIANGLE"); sqrBtn.setButtonText("SQUARE");

    toolButtons = { &pencilBtn, &lineBtn, &arcBtn, &sawBtn, &triBtn, &sqrBtn };
    for (auto* b : toolButtons) {
        addAndMakeVisible(b); b->setClickingTogglesState(true);
        b->setRadioGroupId(radioGroupId);
        b->addListener(this);
    }
    pencilBtn.setToggleState(true, juce::dontSendNotification);
    currentDrawTool = AutoDrawTool::Pencil;

    dummyLane.defaultValue = 0.5f;
}

void AutomationEditor::buttonClicked(juce::Button* button) {
    if (button->getToggleState()) {
        if (button == &pencilBtn) currentDrawTool = AutoDrawTool::Pencil;
        else if (button == &lineBtn) currentDrawTool = AutoDrawTool::SCurve;
        else if (button == &arcBtn) currentDrawTool = AutoDrawTool::Arc;
        else if (button == &sawBtn) currentDrawTool = AutoDrawTool::Saw;
        else if (button == &triBtn) currentDrawTool = AutoDrawTool::Triangle;
        else if (button == &sqrBtn) currentDrawTool = AutoDrawTool::Square;
        currentPreviewCellIdx = -1; repaint();
    }
}

void AutomationEditor::setClipReference(MidiPattern* clip) {
    juce::ScopedLock sl(dataLock);
    clipRef = clip;

    if (clipRef != nullptr) {
        float sX = clipRef->getStartX();
        if (clipRef->autoVol.nodes.empty()) {
            clipRef->autoVol.defaultValue = 0.8f;
            clipRef->autoVol.nodes.push_back({ sX, 0.8f, 0.0f, AutomationMath::SingleCurve });
        }
        if (clipRef->autoPan.nodes.empty()) {
            clipRef->autoPan.defaultValue = 0.5f;
            clipRef->autoPan.nodes.push_back({ sX, 0.5f, 0.0f, AutomationMath::SingleCurve });
        }
        if (clipRef->autoFilter.nodes.empty()) {
            clipRef->autoFilter.defaultValue = 0.5f;
            clipRef->autoFilter.nodes.push_back({ sX, 0.5f, 0.0f, AutomationMath::SingleCurve });
        }
        if (clipRef->autoPitch.nodes.empty()) {
            clipRef->autoPitch.defaultValue = 0.5f;
            clipRef->autoPitch.nodes.push_back({ sX, 0.5f, 0.0f, AutomationMath::SingleCurve });
        }
    }
    repaint();
}

void AutomationEditor::updateView(double hS, double hZ, float vS, float vZ, double snap, float ph) {
    hScroll = hS; hZoom = hZ; vScroll = vS; vZoom = vZ; snapPixels = snap; currentPlayheadX = ph;
    repaint();
}

float AutomationEditor::getVolumeAt(float px) { juce::ScopedLock sl(dataLock); return clipRef ? clipRef->autoVol.getValueAt(px) : 0.8f; }
float AutomationEditor::getPanAt(float px) { juce::ScopedLock sl(dataLock); return clipRef ? clipRef->autoPan.getValueAt(px) : 0.5f; }
float AutomationEditor::getFilterAt(float px) { juce::ScopedLock sl(dataLock); return clipRef ? clipRef->autoFilter.getValueAt(px) : 0.5f; }
float AutomationEditor::getPitchAt(float px) { juce::ScopedLock sl(dataLock); return clipRef ? clipRef->autoPitch.getValueAt(px) : 0.5f; }

AutoLane* AutomationEditor::getCurrentAutoLane() { return getLaneById(autoSelector.getSelectedId()); }
AutoLane* AutomationEditor::getLaneById(int id) {
    if (!clipRef) return &dummyLane;
    if (id == 1) return &clipRef->autoVol;
    if (id == 2) return &clipRef->autoPan;
    if (id == 3) return &clipRef->autoFilter;
    if (id == 4) return &clipRef->autoPitch;
    return &clipRef->autoVol;
}

juce::Colour AutomationEditor::getLaneColor() {
    switch (autoSelector.getSelectedId()) {
    case 1: return juce::Colours::limegreen; case 2: return juce::Colours::cyan; case 3: return juce::Colours::orange; case 4: return juce::Colours::magenta; default: return juce::Colours::white;
    }
}

juce::String AutomationEditor::getFormattedValue(float val) {
    switch (autoSelector.getSelectedId()) {
    case 1: return juce::String(20.0f * std::log10(val * 1.25f + 0.0001f), 1) + " dB";
    case 2: return (val < 0.49f) ? juce::String(juce::roundToInt((0.5f - val) * 200)) + "% L" : (val > 0.51f ? juce::String(juce::roundToInt((val - 0.5f) * 200)) + "% R" : "Center");
    case 3: { float f = 20.0f * std::pow(1000.0f, val); return (f >= 1000) ? juce::String(f / 1000.0f, 1) + " kHz" : juce::String(juce::roundToInt(f)) + " Hz"; }
    case 4: return juce::String((val - 0.5f) * 24.0f, 1) + " st";
    default: return "";
    }
}

float AutomationEditor::getYForValue(float val) { float pH = getHeight() - toolbarH; float center = toolbarH + pH / 2.0f; return center + (0.5f - val) * (pH - 20.0f) * autoVZoom; }
float AutomationEditor::getValueForY(float y) { float pH = getHeight() - toolbarH; if (y < toolbarH) y = toolbarH; float center = toolbarH + pH / 2.0f; return juce::jlimit(0.0f, 1.0f, 0.5f - (y - center) / ((pH - 20.0f) * autoVZoom)); }

void AutomationEditor::grabNodesUnderNotes(const std::set<int>& selectedIndices) {
    juce::ScopedLock sl(dataLock); linkedNodes.clear(); if (clipRef == nullptr || selectedIndices.empty()) return;
    for (int laneId = 1; laneId <= 4; ++laneId) {
        AutoLane* lane = getLaneById(laneId);
        for (size_t nIdx = 0; nIdx < lane->nodes.size(); ++nIdx) {
            float nx = lane->nodes[nIdx].x; bool isInside = false;
            for (int idx : selectedIndices) {
                if (idx < (int)clipRef->getNotes().size()) {
                    const auto& note = clipRef->getNotes()[idx];
                    if (nx >= note.x && nx <= note.x + note.width) { isInside = true; break; }
                }
            }
            if (isInside) linkedNodes.push_back({ laneId, (int)nIdx, nx });
        }
    }
}

void AutomationEditor::moveLinkedNodes(float deltaX) {
    juce::ScopedLock sl(dataLock);
    for (auto& ln : linkedNodes) {
        AutoLane* lane = getLaneById(ln.laneId); float newX = ln.startX + deltaX;
        float minX = (ln.nodeIdx > 0) ? lane->nodes[ln.nodeIdx - 1].x + 0.1f : 0.0f;
        float maxX = (ln.nodeIdx < (int)lane->nodes.size() - 1) ? lane->nodes[ln.nodeIdx + 1].x - 0.1f : (float)(32.0 * 320.0);
        lane->nodes[ln.nodeIdx].x = juce::jlimit(minX, maxX, newX);
    }
    repaint();
}

void AutomationEditor::clearLinkedNodes() { juce::ScopedLock sl(dataLock); linkedNodes.clear(); }

void AutomationEditor::drawShapeAt(float rawAx, float vy) {
    if (currentDrawTool == AutoDrawTool::Pencil || snapPixels <= 1.0) return;

    int cellIdx = (int)(rawAx / snapPixels);
    float cStart = (float)(cellIdx * snapPixels);
    float cEnd = (float)(cStart + snapPixels);
    float cMid = (float)(cStart + snapPixels * 0.5);

    if (cEnd > 32.0f * 320.0f) return;

    AutoLane* lane = getCurrentAutoLane();
    float baseVal = lane->defaultValue;

    lane->nodes.erase(std::remove_if(lane->nodes.begin(), lane->nodes.end(),
        [&](const AutoNode& n) { return n.x >= cStart - 0.01f && n.x <= cEnd + 0.01f; }), lane->nodes.end());

    switch (currentDrawTool) {
    case AutoDrawTool::SCurve:
        lane->nodes.push_back({ cStart, vy, 0.0f, (int)AutomationMath::HalfSine });
        lane->nodes.push_back({ cEnd - 0.1f, baseVal, 0.0f, (int)AutomationMath::Hold });
        lane->nodes.push_back({ cEnd, baseVal, 0.0f, (int)AutomationMath::SingleCurve });
        break;
    case AutoDrawTool::Arc:
        lane->nodes.push_back({ cStart, baseVal, 0.0f, (int)AutomationMath::HalfSine });
        lane->nodes.push_back({ cMid, vy, 0.0f, (int)AutomationMath::HalfSine });
        lane->nodes.push_back({ cEnd, baseVal, 0.0f, (int)AutomationMath::SingleCurve });
        break;
    case AutoDrawTool::Saw:
        lane->nodes.push_back({ cStart, baseVal, 0.0f, (int)AutomationMath::SingleCurve });
        lane->nodes.push_back({ cEnd - 0.1f, vy, 0.0f, (int)AutomationMath::Hold });
        lane->nodes.push_back({ cEnd, baseVal, 0.0f, (int)AutomationMath::SingleCurve });
        break;
    case AutoDrawTool::Triangle:
        lane->nodes.push_back({ cStart, baseVal, 0.0f, (int)AutomationMath::SingleCurve });
        lane->nodes.push_back({ cMid, vy, 0.0f, (int)AutomationMath::SingleCurve });
        lane->nodes.push_back({ cEnd, baseVal, 0.0f, (int)AutomationMath::SingleCurve });
        break;
    case AutoDrawTool::Square:
        lane->nodes.push_back({ cStart, vy, 0.0f, (int)AutomationMath::Hold });
        lane->nodes.push_back({ cMid, baseVal, 0.0f, (int)AutomationMath::Hold });
        lane->nodes.push_back({ cEnd, baseVal, 0.0f, (int)AutomationMath::SingleCurve });
        break;
    default: break;
    }

    std::sort(lane->nodes.begin(), lane->nodes.end());
}

void AutomationEditor::paint(juce::Graphics& g) {
    juce::Colour bgColor = juce::Colours::black.withAlpha(0.85f);
    g.fillAll(bgColor);

    g.setColour(bgColor.darker(0.3f)); g.fillRect(0, 0, getWidth(), toolbarH);
    g.setColour(juce::Colours::darkgrey); g.drawHorizontalLine(toolbarH - 1, 0, (float)getWidth());

    int gridW = getWidth() - keyW - 16;
    double limitX = 32.0 * 320.0;

    AutoLane* lane = getCurrentAutoLane();
    juce::Colour lCol = getLaneColor();

    g.saveState();
    g.reduceClipRegion(keyW, toolbarH, gridW, getHeight() - toolbarH);

    g.setColour(juce::Colours::white.withAlpha(0.04f));
    for (int i = 1; i <= 3; ++i) {
        float valY = getYForValue(i * 0.25f);
        if (valY > toolbarH && valY < getHeight()) g.drawHorizontalLine((int)valY, keyW, getWidth());
    }
    float centerY = getYForValue(0.5f);
    if (centerY > toolbarH && centerY < getHeight()) {
        g.setColour(juce::Colours::white.withAlpha(0.09f));
        g.drawHorizontalLine((int)centerY, keyW, getWidth());
    }

    double visualSnap = snapPixels; if (visualSnap * hZoom < 12.0) visualSnap = 80.0;
    for (double i = 0; i <= limitX; i += visualSnap) {
        int dx = (int)(i * hZoom - hScroll) + keyW;
        if (dx > getWidth() + 100) break;
        bool isCompas = (std::fmod(i, 320.0) < 0.1);
        g.setColour(isCompas ? juce::Colours::white.withAlpha(0.15f) : juce::Colours::white.withAlpha(0.05f));
        g.drawVerticalLine(dx, (float)toolbarH, (float)getHeight());
    }

    if (currentDrawTool != AutoDrawTool::Pencil && !isDraggingNode && isMouseOverPanel && snapPixels > 1.0) {
        if (currentPreviewCellIdx >= 0 && currentMousePos.y >= toolbarH) {
            float cStart = (float)(currentPreviewCellIdx * snapPixels); float cEnd = (float)(cStart + snapPixels);
            float vy = getValueForY((float)currentMousePos.y);

            int xStartDraw = (int)(cStart * hZoom - hScroll) + keyW;
            int xEndDraw = (int)(cEnd * hZoom - hScroll) + keyW;

            if (xStartDraw < getWidth() && xEndDraw >= keyW) {
                juce::Path ghostPath; float baseVal = lane->defaultValue;
                xStartDraw = juce::jlimit(keyW, getWidth(), xStartDraw); xEndDraw = juce::jlimit(keyW, getWidth(), xEndDraw);

                if (currentDrawTool == AutoDrawTool::SCurve || currentDrawTool == AutoDrawTool::Saw) {
                    bool first = true;
                    for (int px = xStartDraw; px <= xEndDraw; px += 2) {
                        double timeX = (double)(px - keyW + hScroll) / hZoom; if (timeX > limitX || timeX > cEnd + 0.1f) break;
                        float t = (timeX - cStart) / (float)snapPixels;
                        float valY = baseVal;

                        if (currentDrawTool == AutoDrawTool::SCurve) valY = AutomationMath::getCurveValue(vy, baseVal, 0.0f, t, AutomationMath::HalfSine);
                        else valY = AutomationMath::getCurveValue(vy, baseVal, 0.0f, t, AutomationMath::SingleCurve);

                        float ny = getYForValue(valY);
                        if (first) { ghostPath.startNewSubPath((float)px, ny); first = false; }
                        else { ghostPath.lineTo((float)px, ny); }
                    }
                }
                else if (currentDrawTool == AutoDrawTool::Square) {
                    ghostPath.startNewSubPath((float)xStartDraw, getYForValue(vy));
                    ghostPath.lineTo((float)(xStartDraw + xEndDraw) / 2.0f, getYForValue(vy));
                    ghostPath.lineTo((float)(xStartDraw + xEndDraw) / 2.0f, getYForValue(baseVal));
                    ghostPath.lineTo((float)xEndDraw, getYForValue(baseVal));
                }
                else {
                    bool first = true;
                    for (int px = xStartDraw; px <= xEndDraw; px += 2) {
                        double timeX = (double)(px - keyW + hScroll) / hZoom; if (timeX > limitX || timeX > cEnd + 0.1f) break;
                        float valY = baseVal;

                        if (currentDrawTool == AutoDrawTool::Arc) {
                            float cMid = (float)(cStart + snapPixels * 0.5);
                            if (timeX <= cMid) valY = AutomationMath::getCurveValue(baseVal, vy, 0.0f, (timeX - cStart) / (float)(snapPixels * 0.5), AutomationMath::HalfSine);
                            else valY = AutomationMath::getCurveValue(vy, baseVal, 0.0f, (timeX - cMid) / (float)(snapPixels * 0.5), AutomationMath::HalfSine);
                        }
                        else if (currentDrawTool == AutoDrawTool::Triangle) {
                            float cMid = (float)(cStart + snapPixels * 0.5);
                            if (timeX <= cMid) valY = AutomationMath::getCurveValue(baseVal, vy, 0.0f, (timeX - cStart) / (float)(snapPixels * 0.5), AutomationMath::SingleCurve);
                            else valY = AutomationMath::getCurveValue(vy, baseVal, 0.0f, (timeX - cMid) / (float)(snapPixels * 0.5), AutomationMath::SingleCurve);
                        }

                        float ny = getYForValue(valY);
                        if (first) { ghostPath.startNewSubPath((float)px, ny); first = false; }
                        else { ghostPath.lineTo((float)px, ny); }
                    }
                }

                g.setColour(lCol.withAlpha(0.1f)); juce::Path ghostFill = ghostPath; ghostFill.lineTo((float)xEndDraw, (float)getHeight()); ghostFill.lineTo((float)xStartDraw, (float)getHeight()); ghostFill.closeSubPath(); g.fillPath(ghostFill);
                g.setColour(lCol.withAlpha(0.4f)); g.strokePath(ghostPath, juce::PathStrokeType(2.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
            }
        }
    }

    if (currentDrawTool != AutoDrawTool::Pencil) {
        g.setColour(juce::Colours::white.withAlpha(0.4f)); g.setFont(juce::Font("Sans-Serif", 13.0f, juce::Font::bold));
        g.drawText("Shape mode. Ghost preview active. Drag to draw, click to repeat. Hold Ctrl for temporary pointer.", keyW + 15, toolbarH + 10, getWidth() - keyW - 30, 20, juce::Justification::topLeft);
    }

    if (clipRef != nullptr && !clipRef->getNotes().empty()) {
        juce::ScopedLock sl(dataLock);
        g.setColour(juce::Colours::white.withAlpha(0.18f));
        float rowH = 22.0f * vZoom; float pH = 600.0f;
        if (getParentComponent() != nullptr) pH = (float)getParentComponent()->getHeight() - (float)getHeight() - 90.0f;

        for (const auto& n : clipRef->getNotes()) {
            int nx = (int)(n.x * hZoom - hScroll) + keyW; 
            int nw = (int)((n.x + n.width) * hZoom - hScroll) + keyW - nx;
            if (nx + nw < keyW || nx > getWidth()) continue;
            float absoluteY = (127 - n.pitch) * rowH; float yInPR = absoluteY - vScroll;
            float normalizedY = yInPR / pH; float ny = normalizedY * getHeight();
            float mappedHeight = std::max(2.0f, (rowH / pH) * getHeight());
            if (ny + mappedHeight > 0 && ny < getHeight()) g.fillRoundedRectangle((float)nx, ny, (float)nw, mappedHeight, 2.0f);
        }
    }

    juce::ScopedLock sl(dataLock);
    if (!lane->nodes.empty()) {
        juce::Path path; juce::Path fillPath; bool first = true;
        for (int px = keyW; px <= getWidth(); px += 2) {
            double timeX = (double)(px - keyW + hScroll) / hZoom; if (timeX > limitX) break;
            float valY = lane->getValueAt(timeX); float ny = getYForValue(valY);
            if (first) { path.startNewSubPath((float)px, ny); fillPath.startNewSubPath((float)px, (float)getHeight()); fillPath.lineTo((float)px, ny); first = false; }
            else { path.lineTo((float)px, ny); fillPath.lineTo((float)px, ny); }
        }
        fillPath.lineTo((float)getWidth(), (float)getHeight()); fillPath.closeSubPath();
        g.setColour(lCol.withAlpha(0.15f)); g.fillPath(fillPath);
        g.setColour(lCol); g.strokePath(path, juce::PathStrokeType(2.5f));

        if (isMouseOverPanel || isDraggingNode) {
            for (size_t i = 0; i < lane->nodes.size(); ++i) {
                int nx = (int)(lane->nodes[i].x * hZoom - hScroll) + keyW;
                float ny = getYForValue(lane->nodes[i].value);

                if (nx >= keyW - 10) {
                    g.setColour(i == (size_t)draggedAutoNodeIdx ? juce::Colours::white : lCol);
                    g.fillEllipse((float)nx - 6, ny - 6.0f, 12.0f, 12.0f);
                    g.setColour(bgColor); g.fillEllipse((float)nx - 3, ny - 3.0f, 6.0f, 6.0f);
                }

                if (i < lane->nodes.size() - 1) {
                    double midTimeX = (lane->nodes[i].x + lane->nodes[i + 1].x) * 0.5;
                    float midValY = lane->getValueAt((float)midTimeX);
                    int mx = (int)(midTimeX * hZoom - hScroll) + keyW;
                    float my = getYForValue(midValY);
                    if (mx >= keyW && mx <= getWidth()) {
                        g.setColour(i == (size_t)draggedTensionNodeIdx ? juce::Colours::white : lCol.withAlpha(0.6f));
                        g.fillEllipse((float)mx - 3.5f, my - 3.5f, 7.0f, 7.0f);
                    }
                }
            }
        }
    }

    // --- NUEVO: SOMBREADO VISUAL DE LOS LIMITES DEL PATRON ---
    if (clipRef != nullptr) {
        int startScreenX = (int)(clipRef->getStartX() * hZoom - hScroll) + keyW;
        int endScreenX = (int)((clipRef->getStartX() + clipRef->getWidth()) * hZoom - hScroll) + keyW;

        g.setColour(juce::Colours::black.withAlpha(0.5f));
        if (startScreenX > keyW) {
            g.fillRect(keyW, toolbarH, startScreenX - keyW, getHeight() - toolbarH);
        }
        if (endScreenX < getWidth() - 16) {
            g.fillRect(endScreenX, toolbarH, getWidth() - endScreenX, getHeight() - toolbarH);
        }
    }

    g.restoreState();

    if (isDraggingNode) {
        juce::String text = getFormattedValue(tooltipValue);

        if (autoSelector.getSelectedId() == 4) {
            double tx = (double)(tooltipPos.x - keyW + hScroll) / hZoom; int baseP = -1;
            if (clipRef != nullptr) {
                for (const auto& n : clipRef->getNotes()) { if (tx >= n.x && tx <= n.x + n.width) { baseP = n.pitch; break; } }
            }
            if (baseP != -1) {
                float semitones = (tooltipValue - 0.5f) * 24.0f;
                int finalP = juce::jlimit(0, 127, baseP + juce::roundToInt(semitones));
                static const juce::String names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
                text += " -> " + names[finalP % 12] + juce::String(finalP / 12);
            }
        }

        g.setFont(juce::Font("Sans-Serif", 13.0f, juce::Font::bold));
        int textWidth = g.getCurrentFont().getStringWidth(text) + 16;
        float boxX = juce::jlimit((float)keyW, (float)getWidth() - textWidth - 5, tooltipPos.x - (textWidth / 2.0f));
        float boxY = std::max(5.0f + toolbarH, tooltipPos.y - 35.0f);
        juce::Rectangle<float> tb(boxX, boxY, (float)textWidth, 22.0f);
        g.setColour(juce::Colours::black.withAlpha(0.9f)); g.fillRoundedRectangle(tb, 4.0f);
        g.setColour(lCol); g.drawRoundedRectangle(tb, 4.0f, 1.0f);
        g.setColour(juce::Colours::white); g.drawText(text, tb, juce::Justification::centred, false);
    }

    int phX = (int)((double)currentPlayheadX * hZoom - hScroll) + keyW;
    if (phX >= keyW && phX <= getWidth() - 16) { g.setColour(juce::Colours::red.withAlpha(0.8f)); g.drawVerticalLine(phX, (float)toolbarH, (float)getHeight()); }

    g.setColour(juce::Colours::black.withAlpha(0.6f)); g.fillRect(0, toolbarH, keyW, getHeight());
    g.setColour(juce::Colours::white.withAlpha(0.7f)); g.setFont(juce::Font("Sans-Serif", 11.0f, juce::Font::bold));

    float topVal = juce::jlimit(0.0f, 1.0f, 0.5f + 0.5f / autoVZoom);
    float botVal = juce::jlimit(0.0f, 1.0f, 0.5f - 0.5f / autoVZoom);
    g.drawText(getFormattedValue(topVal), 5, toolbarH + 35, keyW - 10, 20, juce::Justification::topLeft);
    g.drawText(getFormattedValue(botVal), 5, getHeight() - 25, keyW - 10, 20, juce::Justification::bottomLeft);

    if (autoSelector.getSelectedId() == 4) {
        int currentPlayingPitch = -1;
        if (clipRef != nullptr) {
            for (const auto& n : clipRef->getNotes()) {
                if (currentPlayheadX >= n.x && currentPlayheadX <= n.x + n.width) { currentPlayingPitch = n.pitch; break; }
            }
        }

        juce::Rectangle<int> tunerR(5, toolbarH + (getHeight() - toolbarH) / 2 - 15, keyW - 10, 30);
        g.setColour(juce::Colours::black.withAlpha(0.9f)); g.fillRoundedRectangle(tunerR.toFloat(), 4.0f);
        g.setColour(lCol); g.drawRoundedRectangle(tunerR.toFloat(), 4.0f, 1.5f);

        juce::String nStr = "--"; g.setColour(juce::Colours::white.withAlpha(0.3f));
        if (currentPlayingPitch != -1) {
            float currentPitchAuto = clipRef->autoPitch.getValueAt(currentPlayheadX);
            float semitones = (currentPitchAuto - 0.5f) * 24.0f;
            int finalP = juce::jlimit(0, 127, currentPlayingPitch + juce::roundToInt(semitones));
            static const juce::String names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
            nStr = names[finalP % 12] + juce::String(finalP / 12);
            g.setColour(juce::Colours::white);
        }
        g.setFont(juce::Font("Sans-Serif", 16.0f, juce::Font::bold)); g.drawText(nStr, tunerR, juce::Justification::centred, false);
    }
}

void AutomationEditor::resized() {
    int drawAreaX = keyW;
    autoSelector.setBounds(5, 5, keyW - 10, toolbarH - 10);
    int bw = 70; int gap = 2; int bx = drawAreaX + 15;
    for (auto* b : toolButtons) { b->setBounds(bx, 5, bw, toolbarH - 10); bx += bw + gap; }
}

void AutomationEditor::mouseEnter(const juce::MouseEvent&) { isMouseOverPanel = true; repaint(); }
void AutomationEditor::mouseExit(const juce::MouseEvent&) { isMouseOverPanel = false; currentPreviewCellIdx = -1; repaint(); }

void AutomationEditor::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& w) {
    if (e.mods.isShiftDown()) {
        autoVZoom = juce::jlimit(1.0f, 8.0f, autoVZoom + w.deltaY * 2.0f);
        repaint();
    }
    else {
        if (auto* p = getParentComponent()) p->mouseWheelMove(e.getEventRelativeTo(p), w);
    }
}

void AutomationEditor::mouseMove(const juce::MouseEvent& e) {
    currentMousePos = e.getPosition();

    if (currentDrawTool != AutoDrawTool::Pencil && snapPixels > 1.0 && isMouseOverPanel && !isDraggingNode && e.y >= toolbarH) {
        double rawAx = std::max(0.0, (double)(e.x - keyW + hScroll) / hZoom);
        int cellIdx = (int)(rawAx / snapPixels);

        if (cellIdx != currentPreviewCellIdx) {
            currentPreviewCellIdx = cellIdx;
            repaint();
        }
    }
    else {
        currentPreviewCellIdx = -1;
    }
}

void AutomationEditor::mouseDown(const juce::MouseEvent& e) {
    if (e.x < keyW || e.y < toolbarH) return;
    juce::ScopedLock sl(dataLock);

    mouseDownPos = e.getPosition();
    double rawAx = std::max(0.0, (double)(e.x - keyW + hScroll) / hZoom);
    if (rawAx > 32.0 * 320.0) return;
    float vy = getValueForY(e.y);

    bool isTempPointer = e.mods.isCtrlDown() || e.mods.isCommandDown();

    if (currentDrawTool != AutoDrawTool::Pencil && !isTempPointer && !e.mods.isRightButtonDown()) {
        drawShapeAt(rawAx, vy);
        isDraggingNode = true;
        tooltipPos = e.position; tooltipValue = vy;
        currentPreviewCellIdx = -1;
        if (auto* pr = findParentComponentOfClass<PianoRollComponent>()) pr->notifyPatternEdited();
        repaint();
        return;
    }

    float snappedAx = rawAx;
    if (!e.mods.isAltDown() && snapPixels > 1.0) {
        snappedAx = std::round(rawAx / snapPixels) * snapPixels;
    }

    AutoLane* lane = getCurrentAutoLane();
    draggedAutoNodeIdx = -1; draggedTensionNodeIdx = -1;

    if (!lane->nodes.empty()) {
        for (size_t i = 0; i < lane->nodes.size() - 1; ++i) {
            double midTimeX = (lane->nodes[i].x + lane->nodes[i + 1].x) * 0.5; float midValY = lane->getValueAt((float)midTimeX);
            int mx = (int)(midTimeX * hZoom - hScroll) + keyW; float my = getYForValue(midValY);
            if (std::abs(e.x - mx) < 15 && std::abs(e.y - my) < 15) {
                draggedTensionNodeIdx = (int)i; isDraggingNode = true;
                tooltipPos = e.position; tooltipValue = midValY; repaint(); return;
            }
        }
    }

    if (e.mods.isRightButtonDown()) {
        int clickedNodeIdx = -1;
        for (size_t i = 0; i < lane->nodes.size(); ++i) {
            if (std::abs(lane->nodes[i].x - rawAx) * hZoom < 15.0) { clickedNodeIdx = (int)i; break; }
        }

        if (clickedNodeIdx != -1) {
            juce::PopupMenu m;
            m.addItem(1, "Delete"); m.addSeparator();
            m.addItem(10, "Copy value"); m.addItem(11, "Paste value", copiedNodeValue >= 0.0f, false); m.addSeparator();
            int currentCurve = lane->nodes[clickedNodeIdx].curveType;
            m.addItem(2, "Hold", true, currentCurve == AutomationMath::Hold);
            m.addItem(3, "Single curve", true, currentCurve == AutomationMath::SingleCurve);
            m.addItem(4, "Double curve", true, currentCurve == AutomationMath::DoubleCurve);
            m.addItem(5, "Half sine", true, currentCurve == AutomationMath::HalfSine);
            m.addItem(6, "Stairs", true, currentCurve == AutomationMath::Stairs);
            m.addItem(7, "Pulse", true, currentCurve == AutomationMath::Pulse);
            m.showMenuAsync(juce::PopupMenu::Options(), [this, clickedNodeIdx](int result) {
                if (result == 0) return;
                juce::ScopedLock menuLock(dataLock); AutoLane* currentLane = getCurrentAutoLane();
                if (result == 1) { currentLane->nodes.erase(currentLane->nodes.begin() + clickedNodeIdx); }
                else if (result == 10) { copiedNodeValue = currentLane->nodes[clickedNodeIdx].value; }
                else if (result == 11) { if (copiedNodeValue >= 0.0f) { currentLane->nodes[clickedNodeIdx].value = copiedNodeValue; } }
                else { currentLane->nodes[clickedNodeIdx].curveType = result; }
                if (auto* pr = findParentComponentOfClass<PianoRollComponent>()) pr->notifyPatternEdited();
                repaint();
                });
            return;
        }
    }
    else {
        for (size_t i = 0; i < lane->nodes.size(); ++i) {
            if (std::abs(lane->nodes[i].x - rawAx) * hZoom < 15.0) {
                draggedAutoNodeIdx = (int)i; lane->nodes[i].value = vy; nodeOriginalX = lane->nodes[i].x; break;
            }
        }
        if (draggedAutoNodeIdx == -1) {
            lane->nodes.push_back({ snappedAx, vy, 0.0f, (int)AutomationMath::SingleCurve });
            std::sort(lane->nodes.begin(), lane->nodes.end());
            for (size_t i = 0; i < lane->nodes.size(); ++i) { if (lane->nodes[i].x == snappedAx) draggedAutoNodeIdx = (int)i; }
            nodeOriginalX = snappedAx;
        }
        isDraggingNode = true; tooltipPos = e.position; tooltipValue = vy;
    }
    if (auto* pr = findParentComponentOfClass<PianoRollComponent>()) pr->notifyPatternEdited();
    repaint();
}

void AutomationEditor::mouseDrag(const juce::MouseEvent& e) {
    juce::ScopedLock sl(dataLock); AutoLane* lane = getCurrentAutoLane();
    float vy = getValueForY(e.y);
    double rawAx = juce::jlimit(0.0, (double)(32.0 * 320.0), (double)(e.x - keyW + hScroll) / hZoom);

    bool isTempPointer = e.mods.isCtrlDown() || e.mods.isCommandDown();

    if (currentDrawTool != AutoDrawTool::Pencil && !isTempPointer && !e.mods.isRightButtonDown()) {
        drawShapeAt(rawAx, vy);
        tooltipPos = e.position; tooltipValue = vy;
        currentPreviewCellIdx = -1;
        if (auto* pr = findParentComponentOfClass<PianoRollComponent>()) pr->notifyPatternEdited();
        repaint();
        return;
    }

    if (draggedAutoNodeIdx == -1 && draggedTensionNodeIdx == -1) return;

    if (draggedTensionNodeIdx != -1) {
        AutoNode& n0 = lane->nodes[draggedTensionNodeIdx]; AutoNode& n1 = lane->nodes[draggedTensionNodeIdx + 1];
        if (std::abs(n1.value - n0.value) > 0.001f) {
            float R = (vy - std::min(n0.value, n1.value)) / std::abs(n1.value - n0.value); R = juce::jlimit(0.01f, 0.99f, R); float power = std::log(R) / std::log(0.5f);
            n0.tension = juce::jlimit(-1.0f, 1.0f, std::log(power) / std::log(4.0f));
        }
        isDraggingNode = true; tooltipPos = e.position; tooltipValue = vy;
    }
    else if (draggedAutoNodeIdx != -1) {
        double finalAx = rawAx;
        if (std::abs(e.x - mouseDownPos.x) < 5) { finalAx = (double)nodeOriginalX; }
        else {
            if (!e.mods.isAltDown() && snapPixels > 1.0) { finalAx = std::round(rawAx / snapPixels) * snapPixels; }
        }
        lane->nodes[draggedAutoNodeIdx].value = vy;
        float minX = draggedAutoNodeIdx > 0 ? lane->nodes[draggedAutoNodeIdx - 1].x + 0.1f : 0.0f;
        float maxX = draggedAutoNodeIdx < (int)lane->nodes.size() - 1 ? lane->nodes[draggedAutoNodeIdx + 1].x - 0.1f : (float)(32.0 * 320.0);
        lane->nodes[draggedAutoNodeIdx].x = (float)juce::jlimit((double)minX, (double)maxX, finalAx);
        isDraggingNode = true; tooltipPos = e.position; tooltipValue = vy;
    }
    if (auto* pr = findParentComponentOfClass<PianoRollComponent>()) pr->notifyPatternEdited();
    repaint();
}

void AutomationEditor::mouseUp(const juce::MouseEvent&) { isDraggingNode = false; draggedAutoNodeIdx = -1; draggedTensionNodeIdx = -1; repaint(); }