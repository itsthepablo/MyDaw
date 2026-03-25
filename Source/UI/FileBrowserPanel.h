#pragma once
#include <JuceHeader.h>

class FileBrowserPanel : public juce::Component, public juce::ComboBox::Listener {
public:
    FileBrowserPanel()
        : thread("FileBrowserThread"),
        dirList(nullptr, thread)
    {
        thread.startThread();

        // 1. Selector de discos/carpetas raiz (EL NUEVO EXPLORADOR MEJORADO)
        addAndMakeVisible(driveSelector);
        driveSelector.addListener(this);

        // Llenamos el selector con accesos directos y los discos del sistema
        juce::Array<juce::File> roots;
        juce::File::findFileSystemRoots(roots);
        int id = 1;

        driveSelector.addItem("Carpeta Musica", id++);
        driveSelector.addItem("Carpeta Documentos", id++);
        driveSelector.addItem("Carpeta Descargas", id++);
        driveSelector.addItem("Escritorio", id++);
        driveSelector.addSeparator();

        for (auto root : roots) {
            driveSelector.addItem(root.getFullPathName(), id++);
        }

        driveSelector.setSelectedId(1, juce::dontSendNotification);

        // 2. Arbol de archivos visual
        fileTree = std::make_unique<juce::FileTreeComponent>(dirList);
        addAndMakeVisible(*fileTree);

        // Iniciamos en la carpeta Musica
        dirList.setDirectory(juce::File::getSpecialLocation(juce::File::userMusicDirectory), true, true);
        fileTree->setColour(juce::FileTreeComponent::backgroundColourId, juce::Colour(20, 22, 25));

        // 3. ¡LA CLAVE! Le damos un ID único al arrastrar archivos desde aqui
        fileTree->setDragAndDropDescription("FileBrowserDrag");

        addAndMakeVisible(headerLabel);
        headerLabel.setText("BROWSER", juce::dontSendNotification);
        headerLabel.setJustificationType(juce::Justification::centred);
        headerLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));
        headerLabel.setFont(juce::Font(12.0f, juce::Font::bold));
    }

    ~FileBrowserPanel() override {
        thread.stopThread(1000);
    }

    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override {
        if (comboBoxThatHasChanged == &driveSelector) {
            juce::File newDir;
            int itemId = driveSelector.getSelectedId();

            // Evaluamos la seleccion para navegar
            if (itemId == 1) newDir = juce::File::getSpecialLocation(juce::File::userMusicDirectory);
            else if (itemId == 2) newDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
            else if (itemId == 3) newDir = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Downloads");
            else if (itemId == 4) newDir = juce::File::getSpecialLocation(juce::File::userDesktopDirectory);
            else {
                newDir = juce::File(driveSelector.getText());
            }

            if (newDir.exists()) {
                dirList.setDirectory(newDir, true, true);
            }
        }
    }

    void paint(juce::Graphics& g) override {
        g.fillAll(juce::Colour(20, 22, 25));
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawVerticalLine(getWidth() - 1, 0, (float)getHeight());
    }

    void resized() override {
        auto area = getLocalBounds();
        headerLabel.setBounds(area.removeFromTop(30));

        // Hacemos espacio para el ComboBox arriba
        driveSelector.setBounds(area.removeFromTop(25).reduced(5, 0));
        area.removeFromTop(5);

        fileTree->setBounds(area);
    }

private:
    juce::TimeSliceThread thread;
    juce::DirectoryContentsList dirList;
    std::unique_ptr<juce::FileTreeComponent> fileTree;
    juce::Label headerLabel;
    juce::ComboBox driveSelector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileBrowserPanel)
};