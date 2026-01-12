//
// Created by Arden on 1/12/2026.
//

#ifndef HOMER_RESCALEEDITOR_H
#define HOMER_RESCALEEDITOR_H
#include "juce_gui_basics/juce_gui_basics.h"
#include "InvisibleSlider.h"
struct RescaleParameters;

class RescaleEditor : public juce::Component, public juce::Slider::Listener, public juce::Timer
{
public:
    RescaleEditor(RescaleParameters& p);
    ~RescaleEditor();
private:
    void paint(juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override;
    void sliderValueChanged(juce::Slider*) override;
    void sliderDragEnded(juce::Slider*) override;
    void sliderDragStarted(juce::Slider*) override;

    RescaleParameters& parameters;

    juce::Rectangle<int> drawArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RescaleEditor);

    std::array<InvisibleSlider, 3> sliders;
};

#endif //HOMER_RESCALEEDITOR_H
