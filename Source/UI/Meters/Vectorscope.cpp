#include "Vectorscope.h"
#include "../../Mixer/Data/MixerChannelData.h"

Vectorscope::Vectorscope()
{
    startTimerHz(30);
    for (int i = 0; i < maxPoints; ++i) {
        points[i].x = 0;
        points[i].y = 0;
    }
}

Vectorscope::~Vectorscope()
{
    stopTimer();
}

void Vectorscope::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    drawBackground(g, bounds);

    if (track == nullptr) return;

    juce::Graphics::ScopedSaveState save(g);
    auto scopeRect = bounds.expanded(8, 0);
    g.reduceClipRegion(scopeRect.expanded(0, 15));

    float centerX = (float)scopeRect.getCentreX();
    float centerY = (float)scopeRect.getCentreY();
    float baseScale = (float)scopeRect.getHeight() * 0.5f;
    float scaleX = baseScale * 1.4f; 
    float scaleY = baseScale * 0.95f;

    // Color Magenta/Rosa del plugin original
    g.setColour(juce::Colour(0xFFFF00FF)); 
    
    // Dibujamos segmentos individuales para evitar la línea horizontal de wrap-around del buffer circular
    for (int i = 0; i < maxPoints - 2; i += 2) 
    {
        float l1 = juce::jlimit(-1.0f, 1.0f, points[i].x); 
        float r1 = juce::jlimit(-1.0f, 1.0f, points[i].y);
        float l2 = juce::jlimit(-1.0f, 1.0f, points[i+1].x); 
        float r2 = juce::jlimit(-1.0f, 1.0f, points[i+1].y);

        auto getCoord = [&](float l, float r) -> juce::Point<float> {
            float side = (r - l);
            float mid = (l + r);
            return { centerX + (side * scaleX * 0.707f), centerY - (mid * scaleY * 0.707f) };
        };

        auto p1 = getCoord(l1, r1);
        auto p2 = getCoord(l2, r2);

        // Solo dibujamos si no es el punto de cruce del buffer (evita la línea horizontal parásita)
        if (std::abs(p1.x - p2.x) < (float)getWidth() * 0.5f)
            g.drawLine(p1.x, p1.y, p2.x, p2.y, 1.2f);
    }
}

void Vectorscope::timerCallback()
{
    updatePoints();
    repaint();
}

void Vectorscope::updatePoints()
{
    if (track == nullptr || !isVisible()) return;

    juce::SpinLock::ScopedLockType sl(track->bufferLock);
    
    const auto& buffer = track->midSideBuffer; // Buffer que ahora contiene L/R
    int totalBufferSamples = buffer.getNumSamples();
    int numChannels = buffer.getNumChannels();

    if (totalBufferSamples == 0 || numChannels < 2) return;

    const float* dataL = buffer.getReadPointer(0);
    const float* dataR = buffer.getReadPointer(1);

    int writePos = track->mixerData.scopeWritePos.load(std::memory_order_relaxed);
    
    // Leemos las muestras más recientes en una ventana deslizante de 512 puntos
    for (int i = 0; i < maxPoints; ++i)
    {
        // Calculamos la posición circular hacia atrás desde el índice de escritura
        int readPos = (writePos - maxPoints + i + totalBufferSamples) % totalBufferSamples;
        
        points[i].x = dataL[readPos];
        points[i].y = dataR[readPos];
    }
}

void Vectorscope::drawBackground(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    float centerX = (float)bounds.getCentreX();
    float centerY = (float)bounds.getCentreY();
    float baseScale = (float)bounds.getHeight() * 0.5f;
    float scaleX = baseScale * 1.4f; 
    float scaleY = baseScale * 0.95f;

    // 1. ESTRUCTURA (GRID) - LITERAL TP INSPECTOR
    juce::Path gridPath;
    for (int i = 0; i < 8; ++i)
    {
        float angle = i * juce::MathConstants<float>::pi * 0.25f;
        gridPath.startNewSubPath(centerX, centerY);
        float endX = centerX + std::cos(angle) * scaleX;
        float endY = centerY - std::sin(angle) * scaleY;
        gridPath.lineTo(endX, endY);
    }

    // 2. ESTILO DE PUNTOS - LITERAL TP INSPECTOR
    juce::Path dottedPath;
    float dashPattern[] = { 0.5f, 6.0f };
    juce::PathStrokeType strokeType(2.0f);
    strokeType.createDashedStroke(dottedPath, gridPath, dashPattern, 2);
    strokeType.setEndStyle(juce::PathStrokeType::rounded);

    g.setColour(juce::Colours::white.withAlpha(0.4f)); 
    g.fillPath(dottedPath);

    // 3. ETIQUETAS - LITERAL TP INSPECTOR
    g.setColour(juce::Colours::white.withAlpha(0.4f));
    g.setFont(12.0f);

    auto drawLabel = [&](juce::String text, float angleDeg) {
        float angleRad = angleDeg * (juce::MathConstants<float>::pi / 180.0f);
        float tipX = centerX + std::cos(angleRad) * scaleX;
        float tipY = centerY - std::sin(angleRad) * scaleY;
        float lblX = tipX + std::cos(angleRad) * 20.0f; 
        float lblY = tipY - std::sin(angleRad) * 20.0f;
        g.drawText(text, (int)lblX - 15, (int)lblY - 10, 30, 20, juce::Justification::centred);
    };

    drawLabel("+R", 45.0f);
    drawLabel("+L", 135.0f);
    drawLabel("-R", 225.0f);
    drawLabel("-L", 315.0f);

    // 4. PUNTO CENTRAL
    g.fillRect(centerX - 1.5f, centerY - 1.5f, 3.0f, 3.0f); 
}
