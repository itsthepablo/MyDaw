#include <JuceHeader.h>
#include "../LookAndFeel/RoutingLookAndFeel.h"
#include <functional>

/**
 * Selector modular de Sidechain para ser usado en cabeceras de plugins o paneles de ruteo.
 * Utiliza un puente (Bridge) para obtener los nombres de pista y notificar cambios.
 */
class SidechainSelector : public juce::Component {
public:
    SidechainSelector() {
        // Aplicar estética modular
        setLookAndFeel(&routingLF);

        addAndMakeVisible(label);
        label.setText("SIDECHAIN:", juce::dontSendNotification);
        label.setFont(juce::Font(10.0f, juce::Font::bold));
        label.setColour(juce::Label::textColourId, RoutingLookAndFeel::getSidechainGold().withAlpha(0.8f));

        addAndMakeVisible(selector);
        selector.addItem("None", -100);
        
        selector.onChange = [this] {
            if (onSourceChanged) {
                int id = selector.getSelectedId();
                onSourceChanged(id == -100 ? -1 : id);
            }
        };
    }

    ~SidechainSelector() override {
        setLookAndFeel(nullptr);
    }

    /**
     * Puente: Actualiza la lista de pistas disponibles.
     */
    void updateAvailableTracks(const juce::Array<std::pair<int, juce::String>>& tracks, int currentSourceId) {
        selector.clear(juce::dontSendNotification);
        selector.addItem("None", -100);
        
        for (const auto& t : tracks)
            selector.addItem(t.second, t.first);

        if (currentSourceId == -1) selector.setSelectedId(-100, juce::dontSendNotification);
        else selector.setSelectedId(currentSourceId, juce::dontSendNotification);
    }

    void resized() override {
        auto area = getLocalBounds();
        label.setBounds(area.removeFromLeft(80).reduced(0, 5));
        selector.setBounds(area.reduced(0, 4));
    }

    // Callback para el puente
    std::function<void(int)> onSourceChanged;

private:
    RoutingLookAndFeel routingLF;
    juce::Label label;
    juce::ComboBox selector;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SidechainSelector)
};
