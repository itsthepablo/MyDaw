#pragma once
#include <JuceHeader.h>
#include "../Tracks/Track.h" // REQUERIDO: Para acceder a AudioClipData
#include <vector>
#include <cmath>
#include <algorithm>

/**
 * WaveformRenderer: Implementación Bipolar Simétrica (Estilo FL Studio)
 * OPTIMIZADA: Lee picos pre-calculados (cachedMaxPeaks) para renderizado instantáneo.
 */
class WaveformRenderer {
public:
    static void drawWaveform(juce::Graphics& g,
        const AudioClipData& clipData, // RECIBE: La estructura de datos completa, no el buffer
        juce::Rectangle<int> area,
        juce::Colour baseColor)
    {
        // 1. Acceso al Caché Pre-calculado
        const auto& cache = clipData.cachedMaxPeaks;

        const int width = area.getWidth();
        const int height = area.getHeight();

        // Validación crítica para evitar errores de dibujo o división por cero
        if (cache.empty() || width <= 0 || height <= 0)
            return;

        const float midY = (float)area.getY() + (float)height / 2.0f;
        const float halfHeight = (float)height / 2.0f;

        // 2. Definición Estética: Outline brillante, Relleno al 90% opacidad
        const juce::Colour outlineColor = baseColor.brighter(0.2f).withAlpha(0.9f);
        const juce::Colour fillColor = baseColor.withAlpha(0.9f);

        // 3. Mapeo de Densidad: Puntos de caché por píxel horizontal
        const float pointsPerPixel = (float)cache.size() / (float)width;

        for (int x = 0; x < width; ++x)
        {
            // Calculamos qué índice del caché corresponde a este píxel
            int cacheIdx = (int)(x * pointsPerPixel);

            // Verificación de seguridad de límites del vector
            if (cacheIdx >= (int)cache.size()) break;

            // 4. Lógica de Mirroring (Simetría Perfecta)
            // Leemos el pico máximo absoluto ya calculado al cargar el archivo
            float peak = cache[cacheIdx];

            // Aplicamos un ligero boost visual para mejorar la visibilidad (Opcional)
            peak = juce::jmin(1.0f, peak * 1.05f);

            // 5. Geometría simétrica bipolar
            const float yOffset = peak * halfHeight;
            const float topY = midY - yOffset;
            const float bottomY = midY + yOffset;
            const int currentX = area.getX() + x;

            // 6. DIBUJO ULTRA-RÁPIDO (Cuerpo y Contorno)
            // Dibujamos el relleno sólido
            g.setColour(fillColor);
            g.drawVerticalLine(currentX, topY, bottomY);

            // Dibujamos el outline (puntos brillantes superior e inferior)
            // fillRect de 1x1 es compatible con precisión sub-píxel y antialiasing
            g.setColour(outlineColor);
            g.fillRect((float)currentX, topY, 1.0f, 1.0f);    // Punto superior
            g.fillRect((float)currentX, bottomY, 1.0f, 1.0f); // Punto inferior
        }
    }
};