#pragma once
#include <JuceHeader.h>

// Declaración adelantada (Forward declaration) para evitar referencias circulares
class PlaylistComponent;

class PlaylistDropHandler {
public:
    // Procesa archivos arrastrados desde fuera del DAW (Windows Explorer, Finder, etc.)
    static void processExternalFiles(const juce::StringArray& files, int x, int y, PlaylistComponent& playlist);
    
    // Procesa elementos arrastrados internamente (FileBrowserPanel o PickerPanel)
    static void processInternalItem(const juce::DragAndDropTarget::SourceDetails& dragSourceDetails, PlaylistComponent& playlist);
};