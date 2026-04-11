#include "PluginProcessor.h"
#include "PluginEditor.h"

// --- HELPER PARA OBTENER NOTA (Restaurado) ---
static juce::String getNoteName(float freq)
{
    if (freq <= 0) return "";
    float noteNum = 69 + 12 * std::log2(freq / 440.0f);
    int noteInt = (int)(noteNum + 0.5f);
    const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    if (noteInt < 0) noteInt = 0;
    int octave = (noteInt / 12) - 1;
    int nameIdx = noteInt % 12;
    return juce::String(names[nameIdx]) + juce::String(octave);
}

// --- FUNCIÓN DE DIBUJO ---
static void drawCurve(juce::Graphics& g, const std::vector<float>& data, juce::Rectangle<float> bounds, bool fill, juce::Colour color, float strokeThickness = 1.2f)
{
    int numPoints = (int)data.size();
    if (numPoints < 2) return;

    float minDB = -78.0f;
    float maxDB = -18.0f;

    juce::Path path;
    bool started = false;
    float w = bounds.getWidth();
    float h = bounds.getHeight();
    float startX = bounds.getX();
    float startY = bounds.getY();

    for (float i = 0; i < w; i += 1.0f)
    {
        float normX = i / w;
        float exactIndex = normX * (numPoints - 1);
        int idx = (int)exactIndex;

        if (idx < 0 || idx >= numPoints - 1) continue;

        float mu = exactIndex - idx;
        float db = data[idx] * (1.0f - mu) + data[idx + 1] * mu;

        if (!std::isfinite(db)) db = -144.0f;

        float normY = 1.0f - juce::jmap(db, minDB, maxDB, 0.0f, 1.0f);
        float screenY = startY + (normY * h);

        if (db <= minDB) screenY = startY + h + 10.0f;
        if (screenY < startY - 10.0f) screenY = startY - 10.0f;
        if (screenY > startY + h + 200.0f) screenY = startY + h + 200.0f;

        if (!started) {
            path.startNewSubPath(startX + i, screenY);
            started = true;
        }
        else {
            path.lineTo(startX + i, screenY);
        }
    }

    if (fill)
    {
        juce::Path fillPath = path;
        fillPath.lineTo(startX + w, startY + h + 10.0f);
        fillPath.lineTo(startX, startY + h + 10.0f);
        fillPath.closeSubPath();

        g.setColour(color.withAlpha(0.3f));
        g.fillPath(fillPath);
        g.setColour(color.withAlpha(1.0f));
        g.strokePath(path, juce::PathStrokeType(strokeThickness));
    }
    else
    {
        g.setColour(color);
        g.strokePath(path, juce::PathStrokeType(strokeThickness));
    }
}

// ==============================================================================
TpAnalyzerAudioProcessorEditor::TpAnalyzerAudioProcessorEditor(TpAnalyzerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setOpaque(true);
    setSize(800, 500);

    displaySpectrum.assign(800, -140.0f);
    displayPeak.assign(800, -140.0f);

    // BOTÓN MASTER
    addAndMakeVisible(masterButton);
    masterButton.setButtonText("MASTER");
    masterButton.setRadioGroupId(1);
    masterButton.setClickingTogglesState(true);
    masterButton.setToggleState(false, juce::dontSendNotification);
    masterButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffeebb00));
    masterButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    masterButton.onClick = [this] {
        audioProcessor.analyzer.setMasterMode(true);
        };

    // BOTÓN DEFAULT
    addAndMakeVisible(defaultButton);
    defaultButton.setButtonText("DEFAULT");
    defaultButton.setRadioGroupId(1);
    defaultButton.setClickingTogglesState(true);
    defaultButton.setToggleState(true, juce::dontSendNotification);
    defaultButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::white);
    defaultButton.setColour(juce::TextButton::textColourOnId, juce::Colours::black);
    defaultButton.onClick = [this] {
        audioProcessor.analyzer.setMasterMode(false);
        };

    startTimerHz(60);
}

TpAnalyzerAudioProcessorEditor::~TpAnalyzerAudioProcessorEditor()
{
    stopTimer();
}

void TpAnalyzerAudioProcessorEditor::timerCallback()
{
    audioProcessor.analyzer.process();

    auto& specRef = audioProcessor.analyzer.getSpectrumData();
    auto& peakRef = audioProcessor.analyzer.getPeakData();

    if (displaySpectrum.size() != specRef.size()) displaySpectrum = specRef;
    else std::copy(specRef.begin(), specRef.end(), displaySpectrum.begin());

    if (displayPeak.size() != peakRef.size()) displayPeak = peakRef;
    else std::copy(peakRef.begin(), peakRef.end(), displayPeak.begin());

    repaint();
}

void TpAnalyzerAudioProcessorEditor::mouseMove(const juce::MouseEvent& e) {
    mouseX = e.x;
    mouseY = e.y;
}
void TpAnalyzerAudioProcessorEditor::mouseExit(const juce::MouseEvent& e) { mouseX = -1; }

void TpAnalyzerAudioProcessorEditor::resized()
{
    int buttonWidth = 70;
    int buttonHeight = 20;
    int rightMargin = 10;
    int topMargin = 10;

    defaultButton.setBounds(getWidth() - rightMargin - buttonWidth, topMargin, buttonWidth, buttonHeight);
    masterButton.setBounds(defaultButton.getX() - buttonWidth - 5, topMargin, buttonWidth, buttonHeight);

    bakeBackground();
}

void TpAnalyzerAudioProcessorEditor::bakeBackground()
{
    backgroundCache = juce::Image(juce::Image::ARGB, getWidth(), getHeight(), true);
    juce::Graphics g(backgroundCache);
    g.fillAll(juce::Colour::fromFloatRGBA(0.10f, 0.11f, 0.12f, 1.0f));
}

void TpAnalyzerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.drawImageAt(backgroundCache, 0, 0);

    int leftM = 5;
    int topM = 40;
    int rightM = 5;
    int botM = 5;
    juce::Rectangle<float> area((float)leftM, (float)topM, (float)getWidth() - leftM - rightM, (float)getHeight() - topM - botM);

    // AMBOS MODOS DIBUJAN EN BLANCO AHORA
    if (audioProcessor.analyzer.isMasterMode())
    {
        // En master, el espectro es blanco con relleno. El pico es blanco semitransparente sin relleno.
        drawCurve(g, displaySpectrum, area, true, juce::Colours::white);
        drawCurve(g, displayPeak, area, false, juce::Colours::white.withAlpha(0.6f), 1.0f);
    }
    else
    {
        // En default, dibuja blanco sólido idéntico
        drawCurve(g, displaySpectrum, area, true, juce::Colours::white);
    }

    // CROSSHAIR Y TEXTO DE INFO (Restaurado)
    if (mouseX > leftM && mouseY > topM && mouseY < getHeight() - botM)
    {
        g.setColour(juce::Colours::white.withAlpha(0.3f));
        g.drawVerticalLine(mouseX, area.getY(), area.getBottom());
        g.drawHorizontalLine(mouseY, area.getX(), area.getRight());

        float normY = (mouseY - area.getY()) / area.getHeight();
        float db = juce::jmap(1.0f - normY, -78.0f, -18.0f);
        float normX = (mouseX - area.getX()) / area.getWidth();
        float freq = 20.0f * std::pow(1000.0f, normX);

        juce::String infoText;
        infoText << (int)freq << " Hz   " << getNoteName(freq) << "   " << juce::String(db, 1) << " dB";

        g.setColour(juce::Colours::white);
        g.setFont(16.0f);
        // Colocado arriba a la izquierda donde antes estaban los botones
        g.drawText(infoText, leftM + 10, 10, 300, 20, juce::Justification::left, false);
    }
}