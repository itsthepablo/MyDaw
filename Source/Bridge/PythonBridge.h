#pragma once
#include <JuceHeader.h>
#include "../Data/GlobalData.h"
#include <vector>

/**
 * PythonBridge: Clase encargada de la comunicación entre el DAW y scripts de Python.
 * Utiliza intercambio de archivos JSON y juce::ChildProcess.
 */
class PythonBridge {
public:
    PythonBridge() = default;
    ~PythonBridge() = default;

    /**
     * Ejecuta un script de Python sobre un conjunto de notas.
     * @param scriptName Nombre del archivo .py en la carpeta PythonScripts/
     * @param notes Referencia al vector de notas que se va a modificar.
     * @param extraParams Parámetros adicionales (ej: texto de progresión) para el script.
     * @return true si tuvo éxito, false si hubo errores.
     */
    static bool executeScript(const juce::String& scriptName, std::vector<Note>& notes, const juce::String& extraParams = "");

    static juce::var notesToVar(const std::vector<Note>& notes, const juce::String& extraParams = "");
    static void varToNotes(const juce::var& v, std::vector<Note>& notes);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PythonBridge)
};
