#include "MidiPatternRenderer.h"

void MidiPatternRenderer::drawMidiPattern(juce::Graphics& g, 
                                         const MidiPattern& pattern, 
                                         juce::Rectangle<int> area, 
                                         juce::Colour trackColor,
                                         const juce::String& name,
                                         bool isInlineEditingActive,
                                         double hZoom,
                                         int hS,
                                         double playheadAbsPos,
                                         MidiPatternLookAndFeel* lf)
{
    MidiPatternLookAndFeel defaultLF;
    MidiPatternLookAndFeel& activeLF = lf ? *lf : defaultLF;
    (void)activeLF; // LF is used for potential subclassing; drawing is done inline per original

    g.saveState();

    MidiStyleRecipe recipe = MidiStyleRegistry::getRecipe(pattern.getStyle(), trackColor);

    // --- 1. FONDO DEL CLIP (Sólido o Degradado) ---
    if (recipe.drawBackground) {
        if (recipe.useBackgroundGradient) {
            juce::ColourGradient cg(trackColor.darker(0.6f), (float)area.getX(), (float)area.getY(),
                                    juce::Colour(10, 10, 10), (float)area.getX(), (float)area.getBottom(), false);
            g.setGradientFill(cg);
        } else {
            g.setColour(recipe.backgroundColor);
        }
        g.fillRoundedRectangle(area.toFloat(), 5.0f);
    }

    // Borde de seleccion
    if (pattern.getIsSelected()) {
        g.setColour(juce::Colours::yellow);
        g.drawRoundedRectangle(area.toFloat(), 5.0f, 1.5f);
    }

    // --- 2. CABECERA (Header) y NOMBRE ---
    juce::Rectangle<float> headerRect = area.withHeight(14).toFloat();
    if (recipe.drawHeaderBackground) {
        g.setColour(recipe.backgroundColor.withAlpha(0.4f)); 
        g.fillRoundedRectangle(headerRect, 5.0f);
        
        g.setColour(juce::Colours::black.withAlpha(0.15f));
        g.drawHorizontalLine((int)headerRect.getBottom(), (float)area.getX(), (float)area.getRight());
    }

    // Texto del nombre (color = recipe.noteColor, igual que el original)
    g.setColour(recipe.noteColor);
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(" " + name, headerRect.reduced(3.0f, 0.0f).toNearestInt(),
               juce::Justification::centredLeft, true);

    // --- 3. AREA INTERNA (Culling) ---
    g.reduceClipRegion(area);

    if (isInlineEditingActive) {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.fillRoundedRectangle(area.toFloat(), 4.0f);

        g.setColour(juce::Colours::white.withAlpha(0.05f));
        for (float gridX = pattern.getStartX(); gridX < pattern.getStartX() + pattern.getWidth(); gridX += 20.0f) {
            int sx = area.getX() + (int)((gridX - pattern.getStartX()) * hZoom);
            g.drawVerticalLine(sx, (float)area.getY(), (float)area.getBottom());
        }
    }

    // --- 4. RENDERIZADO MULTICAPA DE AUTOMATIZACION ---
    auto isLaneActive = [](const AutoLane& lane) {
        if (lane.nodes.size() > 1) return true;
        if (lane.nodes.size() == 1 && std::abs(lane.nodes[0].value - lane.defaultValue) > 0.001f) return true;
        return false;
    };

    struct ActiveLane {
        const AutoLane* lane;
        juce::Colour color;
    };
    std::vector<ActiveLane> lanesToDraw;

    if (isLaneActive(pattern.autoVol))    lanesToDraw.push_back({ &pattern.autoVol, juce::Colours::limegreen });
    if (isLaneActive(pattern.autoPan))    lanesToDraw.push_back({ &pattern.autoPan, juce::Colours::cyan });
    if (isLaneActive(pattern.autoFilter)) lanesToDraw.push_back({ &pattern.autoFilter, juce::Colours::orange });
    if (isLaneActive(pattern.autoPitch))  lanesToDraw.push_back({ &pattern.autoPitch, juce::Colours::magenta });

    for (const auto& activeItem : lanesToDraw) {
        if (activeItem.lane->nodes.empty()) continue;

        juce::Path autoPath;
        float startScreenX = (float)area.getX();
        float bottomY = (float)area.getBottom();
        float height = (float)area.getHeight();

        autoPath.startNewSubPath(startScreenX, bottomY);

        float valAtStart = activeItem.lane->getValueAt(pattern.getStartX());
        autoPath.lineTo(startScreenX, bottomY - (valAtStart * height));

        for (const auto& node : activeItem.lane->nodes) {
            float localNodeX = node.x - pattern.getStartX();
            if (localNodeX < 0.0f) continue;
            if (localNodeX > pattern.getWidth()) break;

            float screenX = startScreenX + (localNodeX * (float)hZoom);
            float screenY = bottomY - (node.value * height);
            autoPath.lineTo(screenX, screenY);
        }

        float valAtEnd = activeItem.lane->getValueAt(pattern.getStartX() + pattern.getWidth());
        autoPath.lineTo(startScreenX + (pattern.getWidth() * (float)hZoom), bottomY - (valAtEnd * height));
        autoPath.lineTo(startScreenX + (pattern.getWidth() * (float)hZoom), bottomY);
        autoPath.closeSubPath();

        g.setColour(activeItem.color.withAlpha(0.12f));
        g.fillPath(autoPath);

        g.setColour(activeItem.color.withAlpha(0.5f));
        g.strokePath(autoPath, juce::PathStrokeType(1.2f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
    }

    // --- 5. DIBUJO DE NOTAS MIDI ---
    const auto& notes = pattern.getNotes();
    if (!notes.empty()) {
        int minPitch = 127; int maxPitch = 0;
        for (const auto& n : notes) {
            if (n.pitch < minPitch) minPitch = n.pitch;
            if (n.pitch > maxPitch) maxPitch = n.pitch;
        }
        minPitch = std::max(0, minPitch - 3);
        maxPitch = std::min(127, maxPitch + 3);
        int pitchRange = std::max(12, maxPitch - minPitch);

        for (const auto& note : notes) {
            float normalizedY = 1.0f - ((float)(note.pitch - minPitch) / (float)pitchRange);
            float noteY = (float)area.getY() + 4.0f + (normalizedY * (area.getHeight() - 12.0f));

            float worldX = pattern.getStartX() + (note.x - pattern.getOffsetX());
            int noteScreenX = (int)(worldX * hZoom) - hS;
            int noteScreenW = std::max(3, (int)(note.width * hZoom));

            juce::Rectangle<float> miniNoteRect((float)noteScreenX, noteY, (float)noteScreenW, 5.0f);

            bool isActive = (playheadAbsPos >= worldX && playheadAbsPos <= (worldX + note.width));
            juce::Colour finalNoteColor = isActive ? recipe.activeNoteColor : recipe.noteColor;

            if (isActive && recipe.activeUseGradient) {
                // Gradiente especial para Classic: reflejo blanco sobre negro al sonar
                juce::Colour highlightColor = juce::Colours::white.withAlpha(0.9f);
                juce::ColourGradient cg(highlightColor, miniNoteRect.getX(), miniNoteRect.getY(),
                                        recipe.activeNoteColor, miniNoteRect.getX(), miniNoteRect.getBottom(), false);
                g.setGradientFill(cg);
            }
            else if (recipe.useGradient || (isActive && !recipe.activeUseGradient)) {
                // Gradiente neon para Gradient y activos de otros estilos
                juce::Colour topColor = isActive ? recipe.activeNoteColor : recipe.noteColor.brighter(0.4f);
                juce::ColourGradient cg(topColor, miniNoteRect.getX(), miniNoteRect.getY(),
                                        recipe.activeNoteColor.darker(0.1f), miniNoteRect.getX(), miniNoteRect.getBottom(), false);
                g.setGradientFill(cg);
            } else {
                g.setColour(finalNoteColor);
            }

            g.fillRoundedRectangle(miniNoteRect, recipe.noteCornerRadius);

            g.setColour(isActive ? finalNoteColor.brighter(0.2f) : recipe.noteBorder);
            g.drawRoundedRectangle(miniNoteRect, recipe.noteCornerRadius, 1.0f);
        }
    }
    
    g.restoreState();
}

int MidiPatternRenderer::hitTestInlineNote(const MidiPattern& pattern,
                                           juce::Rectangle<int> clipRect,
                                           int mouseX, int mouseY,
                                           double hZoom, int hS)
{
    const auto& notes = pattern.getNotes();
    if (notes.empty()) return -1;

    int minPitch = 127, maxPitch = 0;
    for (const auto& n : notes) {
        if (n.pitch < minPitch) minPitch = n.pitch;
        if (n.pitch > maxPitch) maxPitch = n.pitch;
    }
    minPitch = std::max(0, minPitch - 3);
    maxPitch = std::min(127, maxPitch + 3);
    int pitchRange = std::max(12, maxPitch - minPitch);

    for (int i = (int)notes.size() - 1; i >= 0; --i) {
        const auto& note = notes[i];
        float normalizedY = 1.0f - ((float)(note.pitch - minPitch) / (float)pitchRange);
        float noteY = (float)clipRect.getY() + 4.0f + (normalizedY * (clipRect.getHeight() - 12.0f));

        float worldX = pattern.getStartX() + (note.x - pattern.getOffsetX());
        int noteScreenX = (int)(worldX * hZoom) - hS;
        int noteScreenW = std::max(3, (int)(note.width * hZoom));

        juce::Rectangle<float> noteRect((float)noteScreenX, noteY, (float)noteScreenW, 5.0f);

        if (noteRect.expanded(2.0f, 4.0f).contains((float)mouseX, (float)mouseY)) {
            return i;
        }
    }
    return -1;
}

void MidiPatternRenderer::drawMidiSummary(juce::Graphics& g, 
                                         const MidiPattern& pattern, 
                                         juce::Rectangle<int> area, 
                                         float hZoom, 
                                         float hS, 
                                         int minP, 
                                         int maxP)
{
    const auto& notes = pattern.getNotes();
    if (notes.empty()) return;

    int pitchRange = std::max(1, maxP - minP);
    g.setColour(juce::Colours::white.withAlpha(0.8f));

    for (const auto& note : notes) {
        float normalizedY = 1.0f - ((float)(note.pitch - minP) / (float)pitchRange);
        float noteY = (float)area.getY() + (normalizedY * (area.getHeight() - 4.0f)) + 2.0f;
        float worldX = pattern.getStartX() + (note.x - pattern.getOffsetX());
        int noteScreenX = (int)(worldX * hZoom) - (int)hS;
        int noteScreenW = std::max(2, (int)(note.width * hZoom));

        if (noteScreenX + noteScreenW > 0 && noteScreenX < (area.getX() + area.getWidth())) {
            g.fillRect((float)noteScreenX, noteY, (float)noteScreenW, 3.0f);
        }
    }
}
