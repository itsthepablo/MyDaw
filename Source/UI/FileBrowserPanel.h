#pragma once
#include <JuceHeader.h>

class FileBrowserPanel : public juce::Component {
public:
    FileBrowserPanel()
        : thread("FileBrowserThread"),
        dirList(nullptr, thread)
    {
        // CORREGIDO: Iniciamos el hilo sin pasarle un número entero para ser compatibles con JUCE 7/8
        thread.startThread();

        // Configuramos el árbol visual
        fileTree = std::make_unique<juce::FileTreeComponent>(dirList);
        addAndMakeVisible(*fileTree);

        // Apuntamos a la carpeta Música del usuario por defecto
        dirList.setDirectory(juce::File::getSpecialLocation(juce::File::userMusicDirectory), true, true);

        // Estilizamos el componente
        fileTree->setColour(juce::FileTreeComponent::backgroundColourId, juce::Colour(20, 22, 25));

        // Activamos el comportamiento de Drag and Drop
        fileTree->setDragAndDropDescription("FILES");

        addAndMakeVisible(headerLabel);
        headerLabel.setText("FILES", juce::dontSendNotification);
        headerLabel.setJustificationType(juce::Justification::centred);
        headerLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));
        headerLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    }

    ~FileBrowserPanel() override {
        thread.stopThread(1000);
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(20, 22, 25));
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawVerticalLine(getWidth() - 1, 0, (float)getHeight());
    }

    void resized() override {
        auto area = getLocalBounds();
        headerLabel.setBounds(area.removeFromTop(30));
        fileTree->setBounds(area);
    }

private:
    juce::TimeSliceThread thread;
    juce::DirectoryContentsList dirList;
    std::unique_ptr<juce::FileTreeComponent> fileTree;
    juce::Label headerLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileBrowserPanel)
};