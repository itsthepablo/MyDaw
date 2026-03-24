#include "CustomTheme.h"

CustomTheme::CustomTheme()
{
    setColour(juce::DocumentWindow::textColourId, juce::Colours::white);
    setColour(juce::DocumentWindow::backgroundColourId, juce::Colour(28, 30, 34));
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(28, 30, 34));

    juce::File docsDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    juce::File skinDir = docsDir.getChildFile("MyDAW_Skins");

    if (!skinDir.exists()) {
        skinDir.createDirectory();
    }

    loadSkinFromFolder(skinDir);
}

void CustomTheme::drawDocumentWindowTitleBar(juce::DocumentWindow& window, juce::Graphics& g,
    int w, int h, int titleSpaceX, int titleSpaceW,
    const juce::Image* icon, bool drawTitleTextOnLeft)
{
    g.fillAll(juce::Colour(20, 22, 25));
    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawHorizontalLine(h - 1, 0, (float)w);

    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(juce::Font("Sans-Serif", 15.0f, juce::Font::bold));
    g.drawText(window.getName(), 0, 0, w, h, juce::Justification::centred);
}

juce::Button* CustomTheme::createDocumentWindowButton(int buttonType)
{
    if (buttonType == juce::DocumentWindow::minimiseButton) {
        auto* btn = new SilentWindowButton("-");
        btn->setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 54));
        btn->setColour(juce::TextButton::textColourOffId, juce::Colours::gold);
        return btn;
    }
    else if (buttonType == juce::DocumentWindow::maximiseButton) {
        auto* btn = new SilentWindowButton(juce::String::fromUTF8("\xe2\x96\xa1"));
        btn->setColour(juce::TextButton::buttonColourId, juce::Colour(45, 48, 54));
        btn->setColour(juce::TextButton::textColourOffId, juce::Colours::limegreen);
        return btn;
    }
    else if (buttonType == juce::DocumentWindow::closeButton) {
        auto* btn = new SilentWindowButton("x");
        btn->setColour(juce::TextButton::buttonColourId, juce::Colour(150, 40, 40));
        btn->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        return btn;
    }
    return nullptr;
}

void CustomTheme::loadSkinFromFolder(const juce::File& skinFolder)
{
    imgKnobFilmstrip = juce::ImageCache::getFromFile(skinFolder.getChildFile("knob_strip.png"));
    imgFaderTrack = juce::ImageCache::getFromFile(skinFolder.getChildFile("fader_track.png"));
    imgFaderThumb = juce::ImageCache::getFromFile(skinFolder.getChildFile("fader_thumb.png"));

    juce::File themeXmlFile = skinFolder.getChildFile("theme.xml");
    skinColors.clear();

    if (themeXmlFile.existsAsFile())
    {
        if (auto xml = juce::XmlDocument::parse(themeXmlFile))
        {
            if (auto* colorsNode = xml->getChildByName("Colors"))
            {
                for (auto* colorNode : colorsNode->getChildIterator())
                {
                    if (colorNode->hasTagName("Color"))
                    {
                        juce::String name = colorNode->getStringAttribute("name");
                        juce::String hex = colorNode->getStringAttribute("hex");
                        if (name.isNotEmpty() && hex.isNotEmpty()) {
                            skinColors[name] = juce::Colour::fromString(hex);
                        }
                    }
                }
            }
        }
    }
}

juce::Colour CustomTheme::getSkinColor(const juce::String& colorName, juce::Colour fallbackColor)
{
    if (skinColors.find(colorName) != skinColors.end())
        return skinColors[colorName];
    return fallbackColor;
}

// ------------------------------------------------------------------------------
// ROTARY SLIDERS (KNOBS) - FILMSTRIP PNG O CÓDIGO C++
// ------------------------------------------------------------------------------
void CustomTheme::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
    const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider)
{
    // PLAN A: TIRA DE PNGs (FILMSTRIP 3D)
    if (imgKnobFilmstrip.isValid())
    {
        int numFrames = 128;
        int frameWidth = imgKnobFilmstrip.getWidth();
        int frameHeight = imgKnobFilmstrip.getHeight() / numFrames;

        int frameIndex = (int)(sliderPos * (numFrames - 1));
        frameIndex = juce::jlimit(0, numFrames - 1, frameIndex);

        // CORRECCIÓN: Calcular el cuadrado perfecto más grande que cabe en el espacio y centrarlo
        int size = juce::jmin(width, height);
        int destX = x + (width - size) / 2;
        int destY = y + (height - size) / 2;

        // Forzar renderizado de alta calidad para que no se pixele al encogerse
        g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

        g.drawImage(imgKnobFilmstrip,
            destX, destY, size, size,                            // Destino: Cuadrado perfecto centrado
            0, frameIndex * frameHeight, frameWidth, frameHeight); // Origen: El cuadro exacto del PNG
        return;
    }

    // PLAN B: MATEMÁTICA C++ (Contingencia)
    if (slider.getName() == "TrackKnob")
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
        auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
        auto toX = bounds.getCentreX();
        auto toY = bounds.getCentreY();
        auto lineW = bounds.getWidth() > 30.0f ? 3.0f : 2.0f;
        auto arcRadius = radius - lineW * 0.5f;

        g.setColour(juce::Colour(40, 42, 45));
        g.fillEllipse(toX - radius, toY - radius, radius * 2.0f, radius * 2.0f);

        juce::Path backgroundArc;
        backgroundArc.addCentredArc(toX, toY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, rotaryEndAngle, true);
        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.strokePath(backgroundArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        if (slider.isEnabled()) {
            float centerAngle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * 0.5f;
            float currentAngle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

            juce::Path valueArc;
            if (slider.getMinimum() < 0.0) {
                valueArc.addCentredArc(toX, toY, arcRadius, arcRadius, 0.0f, centerAngle, currentAngle, true);
            }
            else {
                valueArc.addCentredArc(toX, toY, arcRadius, arcRadius, 0.0f, rotaryStartAngle, currentAngle, true);
            }

            g.setColour(juce::Colour(255, 180, 0));
            g.strokePath(valueArc, juce::PathStrokeType(lineW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.drawLine(toX, toY,
                toX + (radius * 0.75f) * std::cos(currentAngle - juce::MathConstants<float>::halfPi),
                toY + (radius * 0.75f) * std::sin(currentAngle - juce::MathConstants<float>::halfPi),
                1.5f);
        }
    }
    else
    {
        auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(2.0f);
        auto radius = (float)juce::jmin(bounds.getWidth() / 2, bounds.getHeight() / 2);
        auto centreX = bounds.getCentreX();
        auto centreY = bounds.getCentreY();
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

        g.setColour(juce::Colour(30, 33, 36));
        g.fillEllipse(rx, ry, rw, rw);
        g.setColour(juce::Colours::orange);
        juce::Path p;
        p.addRectangle(-1.0f, -radius, 2.0f, radius * 0.7f);
        p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
        g.fillPath(p);
    }
}

void CustomTheme::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float minSliderPos, float maxSliderPos,
    const juce::Slider::SliderStyle style, juce::Slider& slider)
{
    juce::LookAndFeel_V4::drawLinearSlider(g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
}

void CustomTheme::drawButtonBackground(juce::Graphics& g, juce::Button& button,
    const juce::Colour& backgroundColour,
    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat();
    bool isOn = button.getToggleState();

    juce::Colour baseColor = isOn ? juce::Colours::orange : juce::Colour(45, 48, 52);
    if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        baseColor = baseColor.brighter(0.2f);

    g.setColour(baseColor);
    g.fillRoundedRectangle(bounds, 4.0f);

    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
}