#pragma once
#include <JuceHeader.h>

class ToolbarButtons : public juce::Component {
public:
    juce::TextButton pickerBtn{"Pick"}; // NUEVO BOTÓN
    juce::TextButton mixerBtn{"Mixer"}, fxBtn{"Effects"};
    
    juce::TextButton pointerBtn{"Sel"};
    juce::TextButton drawBtn{"Lap"};
    juce::TextButton cutBtn{"Cor"};
    juce::TextButton eraseBtn{"Bor"};
    
    juce::ComboBox snapCombo;

    std::function<void()> onTogglePicker; // NUEVO CALLBACK
    std::function<void()> onToggleMixer, onToggleFx;
    std::function<void(int)> onToolChanged;
    std::function<void(double)> onSnapChanged;

    ToolbarButtons() {
        addAndMakeVisible(pickerBtn);
        pickerBtn.onClick = [this] { if (onTogglePicker) onTogglePicker(); };
        pickerBtn.setTooltip("Mostrar/Ocultar Picker de Patrones");

        addAndMakeVisible(mixerBtn);
        mixerBtn.onClick = [this] { if (onToggleMixer) onToggleMixer(); };

        addAndMakeVisible(fxBtn);
        fxBtn.onClick = [this] { if (onToggleFx) onToggleFx(); };

        addAndMakeVisible(pointerBtn);
        addAndMakeVisible(drawBtn);
        addAndMakeVisible(cutBtn);
        addAndMakeVisible(eraseBtn);

        pointerBtn.setTooltip("Seleccionar y Mover");
        drawBtn.setTooltip("Dibujar (Pronto)");
        cutBtn.setTooltip("Tijera (Cortar Clips)");
        eraseBtn.setTooltip("Borrador");

        pointerBtn.setClickingTogglesState(true);
        drawBtn.setClickingTogglesState(true);
        cutBtn.setClickingTogglesState(true);
        eraseBtn.setClickingTogglesState(true);

        pointerBtn.setRadioGroupId(101);
        drawBtn.setRadioGroupId(101);
        cutBtn.setRadioGroupId(101);
        eraseBtn.setRadioGroupId(101);
        
        pointerBtn.setToggleState(true, juce::dontSendNotification);

        auto toolClick = [this](int id) { if (onToolChanged) onToolChanged(id); };
        pointerBtn.onClick = [this, toolClick] { toolClick(1); };
        drawBtn.onClick = [this, toolClick] { toolClick(2); };
        cutBtn.onClick = [this, toolClick] { toolClick(3); };
        eraseBtn.onClick = [this, toolClick] { toolClick(4); };

        addAndMakeVisible(snapCombo);
        snapCombo.addItem("Libre (None)", 1);
        snapCombo.addItem("1/8 Beat (Fusa)", 2);
        snapCombo.addItem("1/4 Beat (Step)", 3);
        snapCombo.addItem("1/2 Beat (Corchea)", 4);
        snapCombo.addItem("1 Beat (Negra)", 5);
        snapCombo.addItem("1 Bar (Compas)", 6);
        
        snapCombo.setSelectedId(5, juce::dontSendNotification); 
        snapCombo.onChange = [this] {
            if (!onSnapChanged) return;
            switch (snapCombo.getSelectedId()) {
                case 1: onSnapChanged(1.0); break;   
                case 2: onSnapChanged(10.0); break;  
                case 3: onSnapChanged(20.0); break;  
                case 4: onSnapChanged(40.0); break;  
                case 5: onSnapChanged(80.0); break;  
                case 6: onSnapChanged(320.0); break; 
            }
        };
    }

    void resized() override {
        auto area = getLocalBounds().reduced(2);
        
        // Botón Picker a la izquierda
        pickerBtn.setBounds(area.removeFromLeft(60).reduced(2));
        area.removeFromLeft(10); 

        snapCombo.setBounds(area.removeFromLeft(140).reduced(0, 4));
        area.removeFromLeft(10); 

        int btnW = 40; 
        pointerBtn.setBounds(area.removeFromLeft(btnW).reduced(2));
        drawBtn.setBounds(area.removeFromLeft(btnW).reduced(2));
        cutBtn.setBounds(area.removeFromLeft(btnW).reduced(2));
        eraseBtn.setBounds(area.removeFromLeft(btnW).reduced(2));

        area.removeFromLeft(10); 

        fxBtn.setBounds(area.removeFromRight(80).reduced(2));
        mixerBtn.setBounds(area.removeFromRight(80).reduced(2));
    }
};