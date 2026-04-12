#include "PythonBridge.h"

static juce::File savedScriptsDir;

bool PythonBridge::executeScript(const juce::String& scriptName, std::vector<Note>& notes)
{
    // Ruta específica solicitada por el usuario (JUCE 8 compatible)
    juce::File scriptsDir ("D:\\Mis Cosas\\Documents\\MyDAW_Skins\\PythonScripts");

    if (!scriptsDir.isDirectory())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error de Scripts",
            "No se encontró la carpeta en:\n" + scriptsDir.getFullPathName() + 
            "\n\nCrea la carpeta o verifica la ruta para continuar.");
        return false;
    }


    savedScriptsDir = scriptsDir;
    auto scriptFile = scriptsDir.getChildFile(scriptName);
    auto tempFile = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("temp_notes.json");

    if (!scriptFile.existsAsFile())
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", 
            "No se encontró el script: " + scriptName + "\nen: " + scriptsDir.getFullPathName());
        return false;
    }

    // 2. Exportar notas a JSON
    juce::var data = notesToVar(notes);
    juce::String jsonStr = juce::JSON::toString(data);
    tempFile.replaceWithText(jsonStr);

    // 3. Ejecutar comando: python script.py temp.json
    juce::StringArray command;
    command.add("python");
    command.add(scriptFile.getFullPathName());
    command.add(tempFile.getFullPathName());

    juce::ChildProcess process;
    if (process.start(command))
    {
        // Esperar a que termine (con timeout de 5 segundos para seguridad)
        if (process.waitForProcessToFinish(5000))
        {
            if (process.getExitCode() != 0)
            {
                juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error en Python", "El script terminó con errores.");
                return false;
            }
        }
        else
        {
            process.kill();
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "El script de Python tardó demasiado (Timeout).");
            return false;
        }
    }
    else
    {
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Error", "Asegúrate de tener Python instalado y en el PATH.");
        return false;
    }

    // 4. Leer JSON modificado
    juce::var result = juce::JSON::parse(tempFile);
    if (!result.isVoid())
    {
        varToNotes(result, notes);
        return true;
    }

    return false;
}

juce::var PythonBridge::notesToVar(const std::vector<Note>& notes)
{
    juce::Array<juce::var> jsonNotes;
    for (const auto& note : notes)
    {
        auto* noteObj = new juce::DynamicObject();
        noteObj->setProperty("pitch", note.pitch);
        noteObj->setProperty("x", note.x);
        noteObj->setProperty("width", note.width);
        noteObj->setProperty("frequency", note.frequency);
        noteObj->setProperty("velocity", note.velocity);
        jsonNotes.add(noteObj);
    }

    auto* root = new juce::DynamicObject();
    root->setProperty("notes", jsonNotes);
    return root;
}

void PythonBridge::varToNotes(const juce::var& v, std::vector<Note>& notes)
{
    auto* notesList = v.getProperty("notes", juce::var()).getArray();
    if (notesList == nullptr) return;

    notes.clear();
    for (int i = 0; i < notesList->size(); ++i)
    {
        auto& item = notesList->getReference(i);
        Note n;
        n.pitch = (int)item["pitch"];
        n.x = (int)item["x"];
        n.width = (int)item["width"];
        n.frequency = (double)item["frequency"];
        n.velocity = (int)item["velocity"];
        notes.push_back(n);
    }
}
