#pragma once
#include <JuceHeader.h>
#include <cmath>

// NO TOCAR NI SIMPLIFICAR HASTA QUE YO LO ORDENE
namespace AutomationMath
{
    enum CurveType {
        Hold = 2,
        SingleCurve = 3,
        DoubleCurve = 4,
        HalfSine = 5,
        Stairs = 6,
        Pulse = 7
    };

    inline float getInterpolatedProgress(float t, float tension, int type)
    {
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;

        switch (type)
        {
            case Hold:
                return 0.0f;

            case SingleCurve:
            {
                float power = (tension < 0.0f) ? (1.0f + tension * 0.9f) : (1.0f + tension * 9.0f);
                if (power < 0.01f) power = 0.01f;
                return std::pow(t, power);
            }

            case DoubleCurve:
            {
                float power = (tension < 0.0f) ? (1.0f + tension * 0.9f) : (1.0f + tension * 9.0f);
                if (power < 0.01f) power = 0.01f;
                if (t < 0.5f) return 0.5f * std::pow(2.0f * t, power);
                return 1.0f - 0.5f * std::pow(2.0f * (1.0f - t), power);
            }

            case HalfSine:
            {
                float sineT = (1.0f - std::cos(t * juce::MathConstants<float>::pi)) * 0.5f;
                float power = (tension < 0.0f) ? (1.0f + tension * 0.9f) : (1.0f + tension * 9.0f);
                return std::pow(sineT, power);
            }

            case Stairs:
            {
                int steps = (int)juce::jmap(std::abs(tension), 0.0f, 1.0f, 2.0f, 16.0f);
                return std::floor(t * steps) / (float)(steps - 1);
            }

            case Pulse:
            {
                int pulses = (int)juce::jmap(std::abs(tension), 0.0f, 1.0f, 1.0f, 8.0f);
                return (std::fmod(t * pulses, 1.0f) < 0.5f) ? 0.0f : 1.0f;
            }

            default:
                return t; 
        }
    }

    inline float getCurveValue(float startVal, float endVal, float tension, float t, int type)
    {
        float progress = getInterpolatedProgress(t, tension, type);
        return startVal + (endVal - startVal) * progress;
    }
}