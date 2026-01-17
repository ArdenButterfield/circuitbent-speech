//
// Created by Arden on 1/11/2026.
//

#ifndef HOMER_BENDSPANEL_H
#define HOMER_BENDSPANEL_H

#include "../state/HomerState.h"
#include "RescaleEditor.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <vector>

class BendsPanel : public juce::Component, public juce::Slider::Listener, public juce::Timer, public juce::Button::Listener
{
public:
    BendsPanel(HomerState& hs);
    ~BendsPanel() override;

private:
    void timerCallback() override;
    void sliderValueChanged (juce::Slider*) override;
    void buttonClicked(juce::Button*) override;
    void buttonStateChanged(juce::Button*) override;
    void paint(juce::Graphics& g) override;
    void resized() override;

    HomerState& homerState;

    std::vector<juce::AudioParameterFloat*> bendParameters;
    std::vector<juce::AudioParameterBool*> toggleParameters;

    std::vector<std::unique_ptr<juce::Slider>> bendSliders;
    std::vector<std::unique_ptr<juce::Label>> bendSliderLabels;
    std::vector<std::unique_ptr<juce::ToggleButton>> toggleButtons;
    juce::Rectangle<int> sliderZone;
    juce::Rectangle<int> toggleZone;

    RescaleEditor formantFrequencyEditor;
    RescaleEditor formantHeightEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BendsPanel);
};

#endif //HOMER_BENDSPANEL_H
