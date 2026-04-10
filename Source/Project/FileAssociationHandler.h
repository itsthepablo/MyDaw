#pragma once
#include <JuceHeader.h>

class FileAssociationHandler
{
public:
    static void registerDAWExtension()
    {
#if JUCE_WINDOWS
        // 1. Obtenemos el archivo ejecutable actual
        juce::File dawExe = juce::File::getSpecialLocation(juce::File::currentExecutableFile);

        // 2. Llamada corregida con los 6 argumentos exactos que exige la API de JUCE
        juce::WindowsRegistry::registerFileAssociation(
            ".perritogordo",               // 1. fileExtension
            "PerritoGordo.Project",        // 2. symbolicDescription (Identificador interno corto)
            "Proyecto Perrito Gordo DAW",  // 3. fullDescription (Descripcin visible para usuario)
            dawExe,                        // 4. targetExecutable (Debe ser el objeto juce::File directo)
            0,                             // 5. iconResourceNumber (0 usa el icono del exe)
            true                           // 6. registerForCurrentUserOnly (FALTABA ESTE: true evita requerir admin siempre)
        );
#endif
    }
};